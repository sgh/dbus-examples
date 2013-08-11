

dbus-signal: dbus-signal.c
	gcc -o $@ $< `pkg-config --cflags --libs dbus-1 dbus-glib-1`

dbus-ping-send: dbus-ping-send.c
	gcc -o $@ $< `pkg-config --cflags --libs dbus-1 dbus-glib-1`

dbus-ping-listen: dbus-ping-listen.c
	gcc -o $@ $< `pkg-config --cflags --libs dbus-1 dbus-glib-1`

clean:
	rm dbus-signal dbus-ping-listen dbus-ping-send
