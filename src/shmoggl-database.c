/* shmoggl-database.c
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

#include "shmoggl-database.h"

sqlite3 *shmoggl_database;

gchar *scheme = "\
CREATE TABLE IF NOT EXISTS tasks\
(\
  id INTEGER PRIMARY KEY,\
  name TEXT UNIQUE NOT NULL\
);\
\
CREATE TABLE IF NOT EXISTS time_slices\
(\
  id INTEGER PRIMARY KEY,\
  start INTEGER NOT NULL,\
  stop INTEGER NOT NULL,\
  task_id INTEGER REFERENCES tasks ON UPDATE CASCADE ON DELETE CASCADE NOT NULL\
);\
";

void
shmoggl_database_connect ()
{
  int err;
  char *zErrMsg = 0;
  const gchar *dataDir = g_get_user_data_dir ();
  gchar *dbFileName = "main.db";
  gchar *dbFilePath = g_build_filename (dataDir, dbFileName, NULL);
  err = sqlite3_open (dbFilePath, &shmoggl_database);
  if (err) {
    fprintf (stderr, "Can't open database: %s\n", sqlite3_errmsg(shmoggl_database));
    sqlite3_close (shmoggl_database);
  }
  err = sqlite3_exec (shmoggl_database, scheme, NULL, NULL, &zErrMsg);
  if (err != SQLITE_OK) {
    fprintf (stderr, "SQL error: %s\n", zErrMsg);
    sqlite3_free (zErrMsg);
  }
}

void
shmoggl_database_disconnect ()
{
  sqlite3_close (shmoggl_database);
}
