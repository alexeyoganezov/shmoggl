/* shmoggl-time-slice.c
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

#include "shmoggl-time-slice.h"

struct _ShmogglTimeSlice
{
  GObject parent_instance;
  guint   id;
  glong   start;
  glong   stop;
  guint   task_id;
};

typedef enum
{
  PROP_ID = 1,
  PROP_START,
  PROP_STOP,
  PROP_TASK_ID,
  N_PROPERTIES
} ShmogglTimeSliceProperty;

static GParamSpec *obj_properties[N_PROPERTIES] = { NULL, };

static void
shmoggl_time_slice_set_property (GObject      *object,
                                 guint         property_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
  ShmogglTimeSlice *self = SHMOGGL_TIME_SLICE (object);
  switch ((ShmogglTimeSliceProperty) property_id)
    {
    case PROP_ID:
      self->id = g_value_get_uint (value);
      break;
    case PROP_TASK_ID:
      self->task_id = g_value_get_uint (value);
      break;
    case PROP_START:
      self->start = g_value_get_long (value);
      break;
    case PROP_STOP:
      self->stop = g_value_get_long (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
shmoggl_time_slice_get_property (GObject    *object,
                                 guint       property_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
  ShmogglTimeSlice *self = SHMOGGL_TIME_SLICE (object);
  switch ((ShmogglTimeSliceProperty) property_id)
    {
    case PROP_ID:
      g_value_set_uint (value, self->id);
      break;
    case PROP_TASK_ID:
      g_value_set_uint (value, self->task_id);
      break;
    case PROP_START:
      g_value_set_long (value, self->start);
      break;
    case PROP_STOP:
      g_value_set_long (value, self->stop);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
shmoggl_time_slice_class_init (ShmogglTimeSliceClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  object_class->set_property = shmoggl_time_slice_set_property;
  object_class->get_property = shmoggl_time_slice_get_property;
  obj_properties[PROP_ID] =
    g_param_spec_uint ("id",
                       "Time slice id",
                       "",
                       0,
                       G_MAXUINT,
                       0,
                       G_PARAM_READWRITE);
  obj_properties[PROP_TASK_ID] =
    g_param_spec_uint ("task_id",
                       "Task id",
                       "",
                       0,
                       G_MAXUINT,
                       0,
                       G_PARAM_READWRITE);
  obj_properties[PROP_START] =
    g_param_spec_long ("start",
                       "Time slice start",
                       "",
                       0,
                       G_MAXLONG,
                       0,
                       G_PARAM_READWRITE);
  obj_properties[PROP_STOP] =
    g_param_spec_long ("stop",
                       "Time slice stop",
                       "",
                       0,
                       G_MAXLONG,
                       0,
                       G_PARAM_READWRITE);
  g_object_class_install_properties (object_class,
                                     N_PROPERTIES,
                                     obj_properties);
}

static void
shmoggl_time_slice_init (ShmogglTimeSlice *self)
{
  //
}

ShmogglTimeSlice *
shmoggl_time_slice_create (gpointer *task_ptr)
{
  gpointer *task = task_ptr;
  char *zErrMsg = 0;
  int rc;
  char sql[256];
  ShmogglTimeSlice *created_time_slice = NULL;
  guint task_id;
  GTimeVal current_time = { NULL };
  g_get_current_time (&current_time);
  glong start = current_time.tv_sec;
  glong stop = current_time.tv_sec + 1;
  g_object_get (task, "id", &task_id, NULL);
  snprintf (sql, sizeof(sql), "INSERT INTO time_slices (id, start, stop, task_id) VALUES (NULL, %ld, %ld, %d) RETURNING *;", start, stop, task_id);
  sqlite3_stmt *stmt;
  rc = sqlite3_prepare_v2 (shmoggl_database, sql, -1, &stmt, NULL);
  if (rc != SQLITE_OK) {
    fprintf (stderr, "SQL error: %s\n", zErrMsg);
    sqlite3_free (zErrMsg);
  }
  while (sqlite3_step (stmt) == SQLITE_ROW) {
    guint id = sqlite3_column_int (stmt, 0);
    glong start = sqlite3_column_int (stmt, 1);
    glong stop = sqlite3_column_int (stmt, 2);
    guint task_id = sqlite3_column_int (stmt, 3);
    created_time_slice = g_object_new (SHMOGGL_TYPE_TIME_SLICE, "id", id, "start", start, "stop", stop, "task_id", task_id, NULL);
  }
  sqlite3_finalize (stmt);
  return created_time_slice;
}

void
shmoggl_time_slice_save (ShmogglTimeSlice *time_slice)
{
  char *zErrMsg = 0;
  int rc;
  char sql[256];
  guint time_slice_id;
  glong time_slice_stop;
  g_object_get (time_slice, "id", &time_slice_id, "stop", &time_slice_stop, NULL);
  snprintf (sql, sizeof(sql), "UPDATE time_slices SET stop = %ld WHERE id = %d;", time_slice_stop, time_slice_id);
  rc = sqlite3_exec (shmoggl_database, sql, NULL, 0, &zErrMsg);
  if (rc != SQLITE_OK) {
    fprintf (stderr, "SQL error: %s\n", zErrMsg);
    sqlite3_free (zErrMsg);
  }
}

void
shmoggl_time_slice_find (GPtrArray* time_slices)
{
  char *zErrMsg = 0;
  int rc;
  gchar *sql = "SELECT * FROM time_slices ORDER BY id DESC;";
  sqlite3_stmt *stmt;
  rc = sqlite3_prepare_v2 (shmoggl_database, sql, -1, &stmt, NULL);
  if (rc != SQLITE_OK) {
    fprintf (stderr, "SQL error: %s\n", zErrMsg);
    sqlite3_free (zErrMsg);
  }
  while (sqlite3_step (stmt) == SQLITE_ROW) {
    int id = sqlite3_column_int (stmt, 0);
    guint start = sqlite3_column_int (stmt, 1);
    guint stop = sqlite3_column_int (stmt, 2);
    guint task_id = sqlite3_column_int (stmt, 3);
    ShmogglTimeSlice *time_slice = g_object_new (SHMOGGL_TYPE_TIME_SLICE, "id", id, "start", start, "stop", stop, "task_id", task_id, NULL);
    g_ptr_array_add (time_slices, time_slice);
  }
  sqlite3_finalize (stmt);
}

void
shmoggl_time_slice_delete (guint id)
{
  char *zErrMsg = 0;
  int rc;
  char sql[256];
  snprintf (sql, sizeof(sql), "DELETE FROM time_slices WHERE id = %d;", id);
  rc = sqlite3_exec (shmoggl_database, sql, NULL, 0, &zErrMsg);
  if (rc != SQLITE_OK) {
    fprintf (stderr, "SQL error: %s\n", zErrMsg);
    sqlite3_free (zErrMsg);
  }
}

G_DEFINE_TYPE (ShmogglTimeSlice, shmoggl_time_slice, G_TYPE_OBJECT)
