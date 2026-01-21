/**
Copyright (c) 2026 Florian Evaldsson

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not
   claim that you wrote the original software. If you use this software
   in a product, an acknowledgment in the product documentation would be
   appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.
*/
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
