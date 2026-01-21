#include "gedit-gdb.h"
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <tepl/tepl.h>
#include <libpeas/peas-object-module.h>

enum {
    PROP_0,
    PROP_WINDOW,
    N_PROPERTIES
};

static void gedit_gdb_plugin_window_activatable_iface_init(GeditWindowActivatableInterface *iface);

G_DEFINE_DYNAMIC_TYPE_EXTENDED(GeditGdbPlugin, gedit_gdb_plugin, G_TYPE_OBJECT, 0,
                               G_IMPLEMENT_INTERFACE_DYNAMIC(GEDIT_TYPE_WINDOW_ACTIVATABLE,
                                                             gedit_gdb_plugin_window_activatable_iface_init))

static void gedit_gdb_plugin_set_property(GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec) {
    GeditGdbPlugin *self = GEDIT_GDB_PLUGIN(object);
    switch (prop_id) {
        case PROP_WINDOW:
            self->window = GEDIT_WINDOW(g_value_dup_object(value));
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
            break;
    }
}

static void gedit_gdb_plugin_get_property(GObject *object, guint prop_id, GValue *value, GParamSpec *pspec) {
    GeditGdbPlugin *self = GEDIT_GDB_PLUGIN(object);
    switch (prop_id) {
        case PROP_WINDOW:
            g_value_set_object(value, self->window);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
            break;
    }
}

static void send_gdb_command(GeditGdbPlugin *self, const gchar *command) {
    if (self->gdb_stdin > 0) {
        ssize_t n = write(self->gdb_stdin, command, strlen(command));
        n = write(self->gdb_stdin, "\n", 1);
        (void)n;
    }
}

int gedit_goto_file_line_column(GeditWindow *window, GFile *gfile, long line, long character)
{
	GeditTab *tab=gedit_window_get_tab_from_location(window,gfile);
	
	if(tab)
	{
		GeditDocument *doc = gedit_tab_get_document(tab);
	
		GtkTextBuffer *buffer = GTK_TEXT_BUFFER(doc);

		GtkTextIter iter;
		gtk_text_buffer_get_iter_at_line_offset(buffer, &iter, line, character);

		gtk_text_buffer_place_cursor(buffer, &iter);
		
		if (tab)
		{
			gedit_window_set_active_tab(window, tab);
			
			GeditView *view = gedit_tab_get_view(tab);
			GtkTextView *text_view = GTK_TEXT_VIEW(view);
			gtk_text_view_scroll_to_iter(text_view, &iter, 0.1, FALSE, 0.0, 0.0);
		}
	}
	else
	{
		GeditTab *tab=gedit_window_create_tab(window,TRUE);
		
		gedit_tab_load_file(tab,gfile,NULL,line+1,character+1,FALSE);

		if (!tab)
		{
			g_autofree char *uri=g_file_get_uri(gfile);
			g_warning("Failed to open file: %s", uri);
			return -1;
		}
	}
	
	return 0;
}

// Enkel parser för att hitta fil och rad i GDB-svaret
static void parse_gdb_line(GeditGdbPlugin *self, const gchar *line) {
    // Vi letar efter mönstret fullname="/path/to/file.c",line="12"
    const gchar *file_start = strstr(line, "fullname=\"");
    const gchar *line_start = strstr(line, "line=\"");

    if (file_start && line_start) {
        gchar file_path[512] = {0};
        int line_num = 0;

        sscanf(file_start, "fullname=\"%[^\"]\"", file_path);
        sscanf(line_start, "line=\"%d\"", &line_num);

        if (strlen(file_path) > 0 && line_num > 0) {
            g_print("DEBUG: Hoppar till %s rad %d\n", file_path, line_num);
            // Här kan man lägga till kod för att öppna filen i Gedit
            // 1. Skapa ett GFile-objekt från sökvägen
            g_autoptr(GFile) gfile = g_file_new_for_path(file_path);

            // 2. Anropa funktionen för att flytta markören/öppna filen
            // Vi subtraherar ofta 1 från raden då GDB är 1-baserat 
            // och editorer ibland internt är 0-baserade, men prova 
            // line_num direkt först. Column sätter vi till 0.
            gedit_goto_file_line_column(self->window, gfile, (long)line_num-1, 0L);
        }
    }
}

static gboolean on_gdb_output(GIOChannel *source, GIOCondition cond, gpointer data) {
    GeditGdbPlugin *self = GEDIT_GDB_PLUGIN(data);
    gchar *line;
    if (g_io_channel_read_line(source, &line, NULL, NULL, NULL) == G_IO_STATUS_NORMAL) {
        g_print("GDB: %s", line);
        if (g_str_has_prefix(line, "*stopped") || g_str_has_prefix(line, "=thread-selected")) {
            parse_gdb_line(self, line);
        }
        g_free(line);
    }
    return TRUE;
}

static void on_connect_clicked(GtkButton *btn, GeditGdbPlugin *self) {
    gchar *argv[] = {"/usr/bin/gdb", "--interpreter=mi", NULL};
    if (g_spawn_async_with_pipes(NULL, argv, NULL, G_SPAWN_DO_NOT_REAP_CHILD, NULL, NULL,
                                 &self->gdb_pid, &self->gdb_stdin, &self->gdb_stdout, NULL, NULL)) {
        
        self->gdb_out_chan = g_io_channel_unix_new(self->gdb_stdout);
        g_io_add_watch(self->gdb_out_chan, G_IO_IN, on_gdb_output, self);

        send_gdb_command(self, "-target-select remote localhost:1234");
        send_gdb_command(self, "-break-insert main");
        send_gdb_command(self, "-exec-continue");
    }
    else
    {
    	fprintf(stdout,"%s:%d FAIL [WOOF]\n",__FILE__,__LINE__);
    }
}

static void on_step_clicked(GtkButton *btn, GeditGdbPlugin *self) {
    send_gdb_command(self, "-exec-step");
}

static void on_next_clicked(GtkButton *btn, GeditGdbPlugin *self) {
    send_gdb_command(self, "-exec-next");
}

static void on_up_clicked(GtkButton *btn, GeditGdbPlugin *self) {
    send_gdb_command(self, "-interpreter-exec console \"up\"");
}

static void on_down_clicked(GtkButton *btn, GeditGdbPlugin *self) {
    send_gdb_command(self, "-interpreter-exec console \"down\"");
}

static void on_continue_clicked(GtkButton *btn, GeditGdbPlugin *self) {
    send_gdb_command(self, "-exec-continue");
}

static void on_break_clicked(GtkButton *btn, GeditGdbPlugin *self) {
    GtkWidget *dialog;
    GtkWidget *content_area;
    GtkWidget *entry;
    GtkWidget *label;

    // Skapa en dialog
    dialog = gtk_dialog_new_with_buttons("Sätt brytpunkt",
                                         GTK_WINDOW(self->window),
                                         GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                         "_Avbryt", GTK_RESPONSE_CANCEL,
                                         "_Sätt brytpunkt", GTK_RESPONSE_OK,
                                         NULL);

    content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    gtk_box_set_spacing(GTK_BOX(content_area), 10);
    gtk_container_set_border_width(GTK_CONTAINER(content_area), 10);

    label = gtk_label_new("Ange funktion (t.ex. main) eller fil:rad (t.ex. main.c:15):");
    gtk_container_add(GTK_CONTAINER(content_area), label);

    entry = gtk_entry_new();
    gtk_entry_set_activates_default(GTK_ENTRY(entry), TRUE); // Gör att Enter-tangenten trycker på OK
    gtk_container_add(GTK_CONTAINER(content_area), entry);
    
    gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_OK);
    gtk_widget_show_all(dialog);

    // Kör dialogen och vänta på svar
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
        const gchar *location = gtk_entry_get_text(GTK_ENTRY(entry));
        if (location && strlen(location) > 0) {
            gchar *command = g_strdup_printf("-break-insert %s", location);
            send_gdb_command(self, command);
            g_free(command);
            g_print("GDB: Sätter brytpunkt på %s\n", location);
        }
    }

    gtk_widget_destroy(dialog);
}

static void gedit_gdb_plugin_activate(GeditWindowActivatable *activatable) {
    GeditGdbPlugin *self = GEDIT_GDB_PLUGIN(activatable);
    
    self->panel = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    GtkWidget *btn_conn = gtk_button_new_with_label("1. Anslut (localhost:1234)");
    GtkWidget *btn_next = gtk_button_new_with_label("2. Nästa rad (Next)");
    GtkWidget *btn_step = gtk_button_new_with_label("3. Gå in i (Step)");
    GtkWidget *btn_up = gtk_button_new_with_label("4. Upp (Up)");
    GtkWidget *btn_down = gtk_button_new_with_label("5. Ner (Down)");
    GtkWidget *btn_continue = gtk_button_new_with_label("6. Fortsätt (Continue)");
    GtkWidget *btn_break = gtk_button_new_with_label("7. Brytpunkt (Break)");
    
    gtk_box_pack_start(GTK_BOX(self->panel), btn_conn, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(self->panel), btn_next, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(self->panel), btn_step, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(self->panel), btn_up, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(self->panel), btn_down, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(self->panel), btn_continue, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(self->panel), btn_break, FALSE, FALSE, 0);
    
    g_signal_connect(btn_conn, "clicked", G_CALLBACK(on_connect_clicked), self);
    g_signal_connect(btn_next, "clicked", G_CALLBACK(on_next_clicked), self);
    g_signal_connect(btn_step, "clicked", G_CALLBACK(on_step_clicked), self);
    g_signal_connect(btn_up, "clicked", G_CALLBACK(on_up_clicked), self);
    g_signal_connect(btn_down, "clicked", G_CALLBACK(on_down_clicked), self);
    g_signal_connect(btn_continue, "clicked", G_CALLBACK(on_continue_clicked), self);
    g_signal_connect(btn_break, "clicked", G_CALLBACK(on_break_clicked), self);

    gtk_widget_show_all(self->panel);
    
    // Hämta sidopanelen
    TeplPanel *side_panel = gedit_window_get_side_panel(self->window);
    
    if (side_panel != NULL) {
        // Vi använder TEPL_PANEL1 och tepl_panel1_add_component
        // Om TEPL_IS_PANEL1 kraschar tidigare, kör vi en rå cast:
        TeplPanelItem *item = tepl_panel_item_new(self->panel, 
                                                "gedit-gdb-debugger", 
                                                "GDB Debugger","builder-debugger-symbolic",-1);
        // 3. Lägg till objektet i panelen
        tepl_panel_add(side_panel, item);
        
        // TeplPanel tar nu hand om item-objektet, så vi kan släppa vår referens
        g_object_unref(item);
    }
}

static void gedit_gdb_plugin_deactivate(GeditWindowActivatable *activatable) {
    GeditGdbPlugin *self = GEDIT_GDB_PLUGIN(activatable);
    if (self->gdb_pid) {
        send_gdb_command(self, "-gdb-exit");
        g_spawn_close_pid(self->gdb_pid);
    }
    if (self->panel) {
        gtk_widget_destroy(self->panel);
        self->panel = NULL;
    }
}

static void gedit_gdb_plugin_init(GeditGdbPlugin *self) {
    self->gdb_pid = 0;
}

static void gedit_gdb_plugin_class_init(GeditGdbPluginClass *klass) {
    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    object_class->set_property = gedit_gdb_plugin_set_property;
    object_class->get_property = gedit_gdb_plugin_get_property;
    g_object_class_override_property(object_class, PROP_WINDOW, "window");
}

static void gedit_gdb_plugin_class_finalize(GeditGdbPluginClass *klass) {}

static void gedit_gdb_plugin_window_activatable_iface_init(GeditWindowActivatableInterface *iface) {
    iface->activate = gedit_gdb_plugin_activate;
    iface->deactivate = gedit_gdb_plugin_deactivate;
}

G_MODULE_EXPORT void peas_register_types(PeasObjectModule *module) {
    gedit_gdb_plugin_register_type(G_TYPE_MODULE(module));
    peas_object_module_register_extension_type(module, GEDIT_TYPE_WINDOW_ACTIVATABLE, GEDIT_TYPE_GDB_PLUGIN);
}
