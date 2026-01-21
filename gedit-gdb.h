#ifndef GEDIT_GDB_H
#define GEDIT_GDB_H

#include <gtk/gtk.h>
#include <gedit/gedit-window.h>
#include <gedit/gedit-window-activatable.h>

G_BEGIN_DECLS

#define GEDIT_TYPE_GDB_PLUGIN (gedit_gdb_plugin_get_type())
G_DECLARE_FINAL_TYPE(GeditGdbPlugin, gedit_gdb_plugin, GEDIT, GDB_PLUGIN, GObject)

struct _GeditGdbPlugin {
    GObject parent_instance;
    GeditWindow *window; // Nu känd via gedit-window.h
    GtkWidget *panel;    // Nu känd via gtk.h
    
    GPid gdb_pid;
    gint gdb_stdin;
    gint gdb_stdout;
    GIOChannel *gdb_out_chan;
};

G_END_DECLS

#endif
