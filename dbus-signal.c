/*
 * Signals with DBus / DBus GLib API
 * 
 * A program to illustrate how to connect a signal to an event using DBus
 * and the DBus GLib API
 *
 * Author: Akarsh Simha <akarshsimha@gmail.com>
 *
 * This code is licensed under The GNU General Public License (GPL)
 * Anyone is free to distribute, copy, modify and redistribute this code
 */

#include <glib.h>
#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>
#include <stdlib.h>

#define false 0

/* 
 * This program subscribes to the 'NameLost' signal from the DBus daemon,
 * registers for a name and then loses it, to check if the method mapped
 * to the signal is called.
 */

GMainLoop *mainloop;
DBusConnection *con;

/*
 * The onNameLost function is to be connected to the 'NameLost' signal
 * from the DBus daemon (org.freedesktop.DBus), so that it is called
 * when we release a name we may have acquired.
 *
 * The function prints the message "!!! We lost our Name !!!" and causes
 * the program to exit from the GLib Main Loop
 */

void onNameLost() {
	g_print("!!! We lost our Name !!!\n");
	g_main_loop_quit(mainloop);
}

/*
 * The releaseName function is to be called 1000 ms after the program enters
 * the GLib Main Loop. It releases the name that we obtain in main() through
 * a dbus_bus_request_name() call, so that we trigger the DBus 'NameLost'
 * signal.
 *
 * The pointer my_data is a pointer to the DBusGConnection object that
 * represents our connection to the DBus daemon, and is not used, because
 * we've made the corresponding DBusConnection object global.
 * (Not a good idea)
 */

gboolean releaseName(gpointer my_data) {

	DBusError *error;
	int retval;

	error = NULL;
	
	/* Release the name we got earlier (That is, if we got one) */
	retval =  dbus_bus_release_name (con, "org.DBusTest.SignalTest", error);

	switch(retval) {
		case DBUS_RELEASE_NAME_REPLY_RELEASED:
			g_printerr("releaseName(): Name org.DBusTest.SignalTest was released successfully\n");
			break;
		case DBUS_RELEASE_NAME_REPLY_NOT_OWNER:
			g_printerr("releaseName(): Name org.DBusTest.SignalTest is not owned by this app!\n");
			break;
		case DBUS_RELEASE_NAME_REPLY_NON_EXISTENT:
			g_printerr("releaseName(): Name org.DBusTest.SignalTest does not exist!\n");
			break;
		default:
			g_printerr("releaseName(): Unknown exit status %d\n", retval);
	}

	if( retval == -1) {
		if( error == NULL ) {
			g_printerr("releaseName(): Something fishy. We got an exit status of -1 but error == NULL!\n");
		}
		else {
			g_printerr("Could not release name org.DBusTest.SignalTest: %s\n", error -> message);
			g_printerr("This program may not terminate...\n");
			dbus_error_free(error);
		}
		
	}

	/* Return false, so that releaseName() will not be called again by the main loop */
	return false;
}

/* The main() function handles the following:
 * 1. Getting the connection to the DBus daemon using dbus_g_bus_get()
 * 2. Requests the DBus daemon for the name org.DBusTest.SignalTest
 * 3. Registers signal handler for the DBus signal 'NameLost' using the
 *    dbus_g_proxy_connect_signal() method
 * 4. Requests that the releaseName() function be called 1s after entering
 *    the GLib Main Loop.
 * 5. Enters the GLib Main Loop
 *
 *
 * It returns 0 on success and 1 on failure
 *
 */


int main() {
	DBusGProxy *proxy;
	DBusGConnection *sigcon;
	GError *error;
	DBusError *dbuserror;
	int retval;

	g_type_init();
	error = NULL;

	/* Obtain a connection to the Session Bus */
	sigcon = dbus_g_bus_get(DBUS_BUS_SESSION, &error);
	if(!sigcon) {
		g_printerr("Failed to connect to Session bus: %s\n", error -> message);
		g_error_free(error);
		exit(1);
	}

	con = dbus_g_connection_get_connection(sigcon);

	dbuserror = NULL;

	/* Request the DBus daemon for the name org.DBusTest.SignalTest */
	/* WARNING: Note that the function does not do an exit(1) even
	 * when it doesn't get the name. This may lead to the program
	 * not terminating, because the releaseName method will fail
	 * subsequently and onNameLost will not be called. Thus the program
	 * will not exit the main loop
	 */

	retval = dbus_bus_request_name(con, "org.DBusTest.SignalTest", DBUS_NAME_FLAG_ALLOW_REPLACEMENT, dbuserror);
	switch(retval) {
		case -1: {
			g_printerr("Couldn't acquire name org.DBusTest.SignalTest for our connection: %s\n", dbuserror -> message);
			g_printerr("This program may not terminate as a result of this error!\n");
			dbus_error_free(dbuserror);
			break;
		}
		case DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER:
			g_printerr("dbus_bus_request_name(): We now own the name org.DBusTest.SignalTest!\n");
			break;
		case DBUS_REQUEST_NAME_REPLY_IN_QUEUE:
			g_printerr("dbus_bus_request_name(): We are standing in queue for our name!\n");
			break;
		case DBUS_REQUEST_NAME_REPLY_EXISTS:
			g_printerr("dbus_bus_request_name(): :-( The name we asked for already exists!\n");
			break;
		case DBUS_REQUEST_NAME_REPLY_ALREADY_OWNER:
			g_printerr("dbus_bus_request_name(): Eh? We already own this name!\n");
			break;
		default:
			g_printerr("dbus_bus_request_name(): Unknown result = %d\n", retval);
	}

	/* Prepare a main loop */
	mainloop = g_main_loop_new(NULL, false);

	/* Create a proxy object for the DBus daemon and register the signal handler
	 * onNameLost() with the signal 'NameLost' from the DBus daemon */

	proxy = dbus_g_proxy_new_for_name(sigcon, "org.freedesktop.DBus", "/org/freedesktop/DBus", "org.freedesktop.DBus");

	dbus_g_proxy_add_signal(proxy, "NameLost", G_TYPE_STRING, G_TYPE_INVALID);
	dbus_g_proxy_connect_signal(proxy, "NameLost", onNameLost, NULL, NULL);

	/* Set a timeout of 1s to call releaseName() */
	g_timeout_add(1000, releaseName, (gpointer) sigcon);

	/* Enter the Main Loop */
	g_main_loop_run(mainloop);

	/* Cleanup code */
	g_object_unref(proxy);

	return 0;
}
