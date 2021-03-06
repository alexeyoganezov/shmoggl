/* main.c
 *
 * Copyright 2021 Alexey Oganezov
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <glib/gi18n.h>
#include <glib.h>

#include "shmoggl-config.h"
#include "shmoggl-window.h"
#include "shmoggl-database.h"

static void
on_activate (GtkApplication *app)
{
	GtkWindow *window;
	g_assert (GTK_IS_APPLICATION (app));
	window = gtk_application_get_active_window (app);
	if (window == NULL)
		window = g_object_new (SHMOGGL_TYPE_WINDOW,
		                       "application", app,
		                       "default-width", 450,
		                       "default-height", 600,
		                       NULL);
	gtk_window_present (window);
}

int
main (int   argc,
      char *argv[])
{
  shmoggl_database_connect ();
	g_autoptr(GtkApplication) app = NULL;
	bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);
	app = gtk_application_new ("com.github.alexeyoganezov.shmoggl", G_APPLICATION_FLAGS_NONE);
	g_signal_connect (app, "activate", G_CALLBACK (on_activate), NULL);
	int ret = g_application_run (G_APPLICATION (app), argc, argv);
  shmoggl_database_disconnect();
	return ret;
}
