#pragma once

#include <gtk/gtk.h>
#include "dependencies/sqlite/sqlite3.h"

G_BEGIN_DECLS

extern sqlite3* shmoggl_database;
void shmoggl_database_connect();
void shmoggl_database_disconnect();

G_END_DECLS
