#ifndef GEDIT_GDB_H
#define GEDIT_GDB_H

#include <gtk/gtk.h>
#include <gedit/gedit-window.h>
#include <gedit/gedit-window-activatable.h>

G_BEGIN_DECLS

#define GEDIT_TYPE_GDB_PLUGIN (gedit_gdb_plugin_get_type())
G_DECLARE_FINAL_TYPE(GeditGdbPlugin, gedit_gdb_plugin, GEDIT, GDB_PLUGIN, GObject)

typedef enum 
{
	GEDIT_GDB_CONNECTION_STATUS_NOT_CONNECTED,
	GEDIT_GDB_CONNECTION_STATUS_CONNECTING,
	GEDIT_GDB_CONNECTION_STATUS_CONNECTED,
	GEDIT_GDB_CONNECTION_STATUS_LENGTH
}GeditGdbConnectionStatus;

struct _GeditGdbPlugin {
    GObject parent_instance;
    GeditWindow *window; // Nu känd via gedit-window.h
    GtkWidget *panel;    // Nu känd via gtk.h
    
    GPid gdb_pid;
    gint gdb_stdin;
    gint gdb_stdout;
    GIOChannel *gdb_out_chan;
    
    GeditGdbConnectionStatus connected;
};

G_END_DECLS

#endif
