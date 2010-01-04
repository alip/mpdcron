/* vim: set cino= fo=croql sw=8 ts=8 sts=0 noet cin fdm=syntax : */

/*
 * Copyright (c) 2009 Ali Polatel <alip@exherbo.org>
 *
 * This file is part of the mpdcron mpd client. mpdcron is free software;
 * you can redistribute it and/or modify it under the terms of the GNU General
 * Public License version 2, as published by the Free Software Foundation.
 *
 * mpdcron is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 * Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "stats-defs.h"
#include "tokenizer.h"

#include <stdbool.h>
#include <string.h>
#include <unistd.h>

#include <glib.h>
#include <gio/gio.h>

struct host {
	char *name;
	int port;
};

struct buffer {
	gchar *data;
	gsize count;
};

static const char GREETING[] = "OK MPDCRON "PROTOCOL_VERSION"\n";
static GSocketService *server;
static GHashTable *clients;

static void
client_destroy(gpointer data)
{
	struct client *client = (struct client *)data;
	g_object_unref(client->output);
	g_object_unref(client->input);
	g_object_unref(client->stream);
	g_free(client);
}

static void
event_flush(G_GNUC_UNUSED GObject *source, GAsyncResult *result,
		gpointer clientid)
{
	GError *error;
	struct client *client;

	client = g_hash_table_lookup(clients, clientid);
	if (client == NULL) {
		/* Already disconnected.
		 * Nothing left to do.
		 */
		return;
	}

	error = NULL;
	if (!g_output_stream_flush_finish(client->output, result, &error)) {
		mpdcron_log(LOG_WARNING, "Write failed: %s", error->message);
		g_error_free(error);
		g_hash_table_remove(clients, clientid);
		return;
	}
}

static void
event_read_line(G_GNUC_UNUSED GObject *source, GAsyncResult *result,
		gpointer clientid)
{
	gsize length;
	gchar *line;
	GError *error;
	struct client *client;

	client = g_hash_table_lookup(clients, clientid);
	if (client == NULL) {
		/* Already disconnected.
		 * Nothing left to do.
		 */
		return;
	}

	error = NULL;
	if ((line = g_data_input_stream_read_line_finish(client->input,
					result, &length, &error)) == NULL) {
		if (error == NULL) {
			/* Client disconnected */
			mpdcron_log(LOG_DEBUG, "[%d]? Disconnected", GPOINTER_TO_INT(clientid));
			g_hash_table_remove(clients, clientid);
			return;
		}
		mpdcron_log(LOG_WARNING, "[%d] Read failed: %s",
				GPOINTER_TO_INT(clientid),
				error ? error->message : "unknown");
		g_error_free(error);
		g_hash_table_remove(clients, clientid);
		return;
	}

	mpdcron_log(LOG_DEBUG, "[%d]< %s", GPOINTER_TO_INT(clientid), line);
	command_process(client, line);
	g_free(line);

	/* Schedule another read */
	g_data_input_stream_read_line_async(client->input,
			G_PRIORITY_DEFAULT, NULL, event_read_line, GINT_TO_POINTER(client->id));
}

static gboolean
event_incoming(G_GNUC_UNUSED GSocketService *srv, GSocketConnection *conn,
		G_GNUC_UNUSED GObject *source, G_GNUC_UNUSED gpointer userdata)
{
	unsigned num_clients;
	struct client *client;

	num_clients = g_hash_table_size(clients);
	if (num_clients >= (unsigned)globalconf.max_connections) {
		mpdcron_log(LOG_WARNING, "Maximum connections reached!");
		return TRUE;
	}
	mpdcron_log(LOG_DEBUG, "[%d]! Connected", num_clients);

	/* Prepare struct client */
	client = g_new(struct client, 1);
	client->id = num_clients;
	client->perm = globalconf.default_permissions;
	client->stream = G_IO_STREAM(conn);

	client->input = g_data_input_stream_new(g_io_stream_get_input_stream(client->stream));
	g_data_input_stream_set_newline_type(client->input, G_DATA_STREAM_NEWLINE_TYPE_LF);

	client->output = g_buffered_output_stream_new(g_io_stream_get_output_stream(client->stream));
	g_buffered_output_stream_set_auto_grow(G_BUFFERED_OUTPUT_STREAM(client->output), TRUE);

	g_hash_table_insert(clients, GINT_TO_POINTER(client->id), client);

	/* Increase reference count of the stream,
	 * We'll free it manually on when client disconnects.
	 */
	g_object_ref(G_OBJECT(client->stream));

	/* Schedule to send greeting */
	server_schedule_write(client, GREETING, sizeof(GREETING) - 1);
	server_flush_write(client);

	/* Schedule read */
	g_data_input_stream_read_line_async(client->input, G_PRIORITY_DEFAULT,
			NULL, event_read_line, GINT_TO_POINTER(client->id));

	return FALSE;
}

static void
bind_callback(gpointer data, gpointer userdata)
{
	char *addr_string;
	GInetAddress *addr;
	GSocketAddress *saddr;
	GError *error;
	struct host *host;

	addr = (GInetAddress *)data;
	host = (struct host *)userdata;

	addr_string = g_inet_address_to_string(addr);
	mpdcron_log(LOG_DEBUG, "Resolved `%s' to %s", host->name, addr_string);

	saddr = g_inet_socket_address_new(addr, host->port);
	error = NULL;
	if (!g_socket_listener_add_address(G_SOCKET_LISTENER(server), saddr,
				G_SOCKET_TYPE_STREAM, G_SOCKET_PROTOCOL_TCP,
				NULL, NULL, &error)) {
		mpdcron_log(LOG_WARNING, "Failed bind to address %s:%d: %s",
				addr_string, host->port, error->message);
		g_error_free(error);
		g_free(addr_string);
		g_object_unref(saddr);
		return;
	}
	mpdcron_log(LOG_DEBUG, "Successful bind to %s:%d",
			addr_string, host->port);
	g_free(addr_string);
	g_object_unref(saddr);
}

static void
event_resolve(GObject *source, GAsyncResult *result, gpointer userdata)
{
	GError *error;
	GList *addrs;
	GResolver *resolver;
	struct host *host;

	resolver = G_RESOLVER(source);
	host = (struct host *)userdata;

	error = NULL;
	addrs = g_resolver_lookup_by_name_finish(resolver, result, &error);
	g_object_unref(resolver);

	if (error != NULL) {
		mpdcron_log(LOG_WARNING, "Resolving hostname %s failed: %s",
				host->name, error->message);
		g_error_free(error);
		g_free(host->name);
		g_free(host);
		return;
	}

	g_list_foreach(addrs, bind_callback, host);
	g_free(host->name);
	g_free(host);
	g_resolver_free_addresses(addrs);
}

void
server_init(void)
{
	g_type_init();
	server = g_socket_service_new();
}

void
server_bind(const char *hostname, int port)
{
	GError *error;
	GSocketAddress *addr;

	if (port == -1) {
#if HAVE_GIO_UNIX
		/* Unix socket */
		unlink(hostname);
		error = NULL;
		addr = g_unix_socket_address_new(hostname);
		if (!g_socket_listener_add_address(G_SOCKET_LISTENER(server),
					G_SOCKET_ADDRESS(addr),
					G_SOCKET_TYPE_STREAM,
					G_SOCKET_PROTOCOL_DEFAULT,
					NULL, NULL, &error)) {
			mpdcron_log(LOG_WARNING, "Failed bind to UNIX socket `%s': %s",
					hostname, error->message);
			g_error_free(error);
			g_object_unref(addr);
			return;
		}
		g_object_unref(addr);
		mpdcron_log(LOG_DEBUG, "Successful bind to %s", hostname);
#else
		mpdcron_log(LOG_WARNING, "No support for Unix sockets");
#endif /* HAVE_GIO_UNIX */
	}
	else if (hostname == NULL) {
		/* Bind on any interface. */
		error = NULL;
		if (!g_socket_listener_add_inet_port(G_SOCKET_LISTENER(server),
					port, NULL, &error)) {
			mpdcron_log(LOG_WARNING, "Failed bind to 0.0.0.0:%d: %s",
					port, error->message);
			g_error_free(error);
		}
		mpdcron_log(LOG_DEBUG, "Successful bind to 0.0.0.0:%d", port);
	}
	else {
		/* Resolve the given host and bind to it. */
		GResolver *resolver;
		struct host *host;

		host = g_new(struct host, 1);
		host->name = g_strdup(hostname);
		host->port = port;

		resolver = g_resolver_get_default();
		g_resolver_lookup_by_name_async(resolver, hostname, NULL,
				event_resolve, host);
	}
}

void
server_start(void)
{
	g_signal_connect(server, "incoming", G_CALLBACK(event_incoming), NULL);
	g_socket_service_start(server);
	clients = g_hash_table_new_full(g_direct_hash, g_direct_equal, NULL, client_destroy);
}

void
server_close(void)
{
	g_socket_service_stop(server);
	g_object_unref(server);
	g_hash_table_destroy(clients);
}

void
server_schedule_write(struct client *client, const gchar *data, gsize count)
{
	/* Since we're writing to a BufferedOutputStream that autogrows this
	 * call shouldn't fail or block.
	 */
	g_output_stream_write(client->output, data, count, NULL, NULL);
}

void
server_flush_write(struct client *client)
{
	g_output_stream_flush_async(client->output, G_PRIORITY_DEFAULT,
			NULL, event_flush, GINT_TO_POINTER(client->id));
}
