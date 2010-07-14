# Makefile for example module
example.so: example.c
	$(CC) -fPIC -shared -Wall -W -Wextra \
		$(CFLAGS) $(LDFLAGS) \
		$(shell pkg-config --cflags --libs glib-2.0) \
		$(shell pkg-config --cflags --libs libmpdclient) \
		$< -o $@
clean:
	rm example.so || true
install:
	install -m644 -s example.so ~/.mpdcron/modules
uninstall:
	rm ~/.mpdcron/modules/example.so || true
