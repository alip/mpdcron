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
#include "fifo_buffer.h"
#include "tokenizer.h"

#include <stdbool.h>
#include <string.h>
#include <unistd.h>

#include <glib.h>
#include <gio/gio.h>

#define BUFFER_SIZE	(4096)

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
client_queue_free_callback(gpointer data, G_GNUC_UNUSED gpointer userdata)
{
	struct buffer *buf = (struct buffer *)data;
	g_free(buf->data);
	g_free(buf);
}

static void
client_destroy(gpointer data)
{
	struct client *client = (struct client *)data;
	g_queue_foreach(client->queue, client_queue_free_callback, NULL);
	g_queue_free(client->queue);
	g_free(client->fifo);
	g_object_unref(client->stream);
	g_free(client);
}

static void
event_write(G_GNUC_UNUSED GObject *source, GAsyncResult *res,
		gpointer clientid)
{
	gssize count;
	GError *error;
	struct buffer *buf;
	struct client *client;

	client = g_hash_table_lookup(clients, clientid);
	if (client == NULL) {
		/* Already disconnected.
		 * Nothing left to do.
		 */
		return;
	}

	error = NULL;
	if ((count = g_output_stream_write_finish(client->output, res, &error)) < 0) {
		mpdcron_log(LOG_WARNING, "Write failed: %s", error->message);
		g_error_free(error);
		g_hash_table_remove(clients, clientid);
		return;
	}

	client->sending = false;
	if (g_queue_get_length(client->queue) > 0) {
		/* There's data waiting to be sent. */
		buf = (struct buffer *) g_queue_pop_head(client->queue);
		server_schedule_write(client, buf->data, buf->count);
		g_free(buf->data);
		g_free(buf);
	}
}

static void
event_read(G_GNUC_UNUSED GObject *source, GAsyncResult *result,
		gpointer clientid)
{
	size_t length;
	gssize count;
	const char *p, *newline;
	char *line, *buffer;
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
	if ((count = g_input_stream_read_finish(client->input, result, &error)) < 0) {
		mpdcron_log(LOG_WARNING, "[%d] Read failed: %s",
				GPOINTER_TO_INT(clientid), error->message);
		g_error_free(error);
		g_hash_table_remove(clients, clientid);
		return;
	}
	else if (count == 0) {
		/* Client disconnected */
		mpdcron_log(LOG_DEBUG, "[%d]? Disconnected", GPOINTER_TO_INT(clientid));
		g_hash_table_remove(clients, clientid);
		return;
	}

	/* Commit the write operation */
	fifo_buffer_append(client->fifo, count);

	p = fifo_buffer_read(client->fifo, &length);
	if (p == NULL)
		goto again;

	newline = memchr(p, '\n', length);
	if (newline == NULL)
		goto again;

	line = g_strndup(p, newline - p);
	fifo_buffer_consume(client->fifo, newline - p + 1);
	g_strchomp(line);
	mpdcron_log(LOG_DEBUG, "[%d]< %s", GPOINTER_TO_INT(clientid), line);
	command_process(client, line);
	g_free(line);

again:
	/* Prepare buffer */
	if ((buffer = fifo_buffer_write(client->fifo, &length)) == NULL) {
		mpdcron_log(LOG_WARNING, "Buffer overflow, closing connection");
		g_hash_table_remove(clients, clientid);
		return;
	}

	/* Schedule another read */
	g_input_stream_read_async(client->input, buffer,
			length, G_PRIORITY_DEFAULT, NULL,
			event_read, clientid);
}

static gboolean
event_incoming(G_GNUC_UNUSED GSocketService *srv, GSocketConnection *conn,
		G_GNUC_UNUSED GObject *source, G_GNUC_UNUSED gpointer userdata)
{
	char *buffer;
	size_t maxsize;
	unsigned num_clients;
	struct client *client;

	num_clients = g_hash_table_size(clients);
	if (num_clients >= CLIENT_MAX) {
		mpdcron_log(LOG_WARNING, "Maximum connections reached!");
		return TRUE;
	}
	mpdcron_log(LOG_DEBUG, "[%d]! Connected", num_clients);

	/* Prepare struct client */
	client = g_new(struct client, 1);
	client->id = num_clients;
	client->perm = globalconf.default_permissions;
	client->sending = false;
	client->queue = g_queue_new();
	client->stream = G_IO_STREAM(conn);
	client->input = g_io_stream_get_input_stream(client->stream);
	client->output = g_io_stream_get_output_stream(client->stream);
	client->fifo = fifo_buffer_new(BUFFER_SIZE);
	g_hash_table_insert(clients, GINT_TO_POINTER(client->id), client);

	/* Increase reference count of the stream,
	 * We'll free it manually on when client disconnects.
	 */
	g_object_ref(G_OBJECT(client->stream));

	/* Schedule to send greeting */
	server_schedule_write(client, GREETING, sizeof(GREETING) - 1);

	/* Prepare buffer */
	if ((buffer = fifo_buffer_write(client->fifo, &maxsize)) == NULL) {
		mpdcron_log(LOG_WARNING, "[%d] Buffer overflow, "
				"closing connection", num_clients);
		g_hash_table_remove(clients, GINT_TO_POINTER(client->id));
		return TRUE;
	}

	/* Schedule read */
	g_input_stream_read_async(client->input, buffer,
			maxsize, G_PRIORITY_DEFAULT, NULL,
			event_read,
			GINT_TO_POINTER(client->id));
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
	struct buffer *buf;

	if (!client->sending) {
		g_output_stream_write_async(client->output, data, count,
			G_PRIORITY_DEFAULT, NULL, event_write, GINT_TO_POINTER(client->id));
		client->sending = true;
	}
	else {
		/* There's already data to be sent and
		 * calling g_output_write_stream() will return
		 * G_IO_ERROR_PENDING. */
		buf = g_new(struct buffer, 1);
		buf->data = g_strndup(data, count);
		buf->count = count;
		g_queue_push_tail(client->queue, buf);
	}
}
