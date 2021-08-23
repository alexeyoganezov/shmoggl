/* shmoggl-task.c
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

#include "shmoggl-task.h"

struct _ShmogglTask
{
  GObject      parent_instance;
  // Database fields
  guint id;
  const gchar *name;
  // Relations
  GPtrArray   *time_slices;
  // Additional fields
  gboolean     is_tracked;
};

typedef enum
{
  PROP_ID = 1,
  PROP_NAME,
  PROP_TIME_SLICES,
  PROP_IS_TRACKED,
  N_PROPERTIES
} ShmogglTaskProperty;

static GParamSpec *obj_properties[N_PROPERTIES] = { NULL, };

typedef enum
{
  TASK_TRACKING_STARTED,
  TASK_TRACKING_STOPPED,
  TASK_UPDATED,
  N_SIGNALS
} ShmogglTaskSignal;

static guint obj_signals[N_SIGNALS] = { 0 };

static void
shmoggl_task_set_property (GObject      *object,
                           guint         property_id,
                           const GValue *value,
                           GParamSpec   *pspec)
{
  ShmogglTask *self = SHMOGGL_TASK (object);
  switch ((ShmogglTaskProperty) property_id)
    {
    case PROP_ID:
      self->id = g_value_get_uint (value);
      break;
    case PROP_NAME:
      g_free (self->name);
      self->name = g_value_dup_string (value);
      break;
    case PROP_TIME_SLICES:
      self->time_slices = g_value_get_pointer (value);
      break;
    case PROP_IS_TRACKED:
      self->is_tracked = g_value_get_boolean (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
shmoggl_task_get_property (GObject    *object,
                           guint       property_id,
                           GValue     *value,
                           GParamSpec *pspec)
{
  ShmogglTask *self = SHMOGGL_TASK (object);
  switch ((ShmogglTaskProperty) property_id)
    {
    case PROP_ID:
      g_value_set_uint (value, self->id);
      break;
    case PROP_NAME:
      g_value_set_string (value, self->name);
      break;
    case PROP_TIME_SLICES:
      g_value_set_pointer (value, self->time_slices);
      break;
    case PROP_IS_TRACKED:
      g_value_set_boolean (value, self->is_tracked);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
shmoggl_task_class_init (ShmogglTaskClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  object_class->set_property = shmoggl_task_set_property;
  object_class->get_property = shmoggl_task_get_property;
  obj_properties[PROP_ID] =
    g_param_spec_uint ("id",
                       "Task id",
                       "",
                       0,
                       G_MAXUINT,
                       0,
                       G_PARAM_READWRITE);
  obj_properties[PROP_NAME] =
    g_param_spec_string ("name",
                         "Task name",
                         "",
                         NULL,
                         G_PARAM_READWRITE);
  obj_properties[PROP_TIME_SLICES] =
    g_param_spec_pointer ("time_slices",
                          "",
                          "",
                          G_PARAM_READWRITE);
  obj_properties[PROP_IS_TRACKED] =
    g_param_spec_boolean ("is_tracked",
                          "Is time tracking started for this task",
                          "",
                          FALSE,
                          G_PARAM_READWRITE);
  g_object_class_install_properties (object_class,
                                     N_PROPERTIES,
                                     obj_properties);
  obj_signals[TASK_TRACKING_STARTED] =
    g_signal_new ("tracking_started",
                  G_OBJECT_CLASS_TYPE (object_class),
                  G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
                  0,
                  NULL,
                  NULL,
                  NULL,
                  G_TYPE_NONE,
                  0);
  obj_signals[TASK_TRACKING_STOPPED] =
    g_signal_new ("tracking_stopped",
                  G_OBJECT_CLASS_TYPE (object_class),
                  G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
                  0,
                  NULL,
                  NULL,
                  NULL,
                  G_TYPE_NONE,
                  0);
  obj_signals[TASK_UPDATED] =
    g_signal_new ("task_updated",
                  G_OBJECT_CLASS_TYPE (object_class),
                  G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
                  0,
                  NULL,
                  NULL,
                  NULL,
                  G_TYPE_NONE,
                  0);
}

static void
shmoggl_task_init (ShmogglTask *self)
{
  self->time_slices = g_ptr_array_new ();
}

ShmogglTask *
shmoggl_task_create (const gchar* taskName)
{
  ShmogglTask *task = NULL;
  char *zErrMsg = 0;
  int rc;
  char sql[256];
  snprintf (sql, sizeof(sql), "INSERT INTO tasks (id, name) VALUES (NULL, '%s') RETURNING *;", taskName);
  sqlite3_stmt *stmt;
  rc = sqlite3_prepare_v2 (shmoggl_database, sql, -1, &stmt, NULL);
  if (rc != SQLITE_OK) {
    fprintf (stderr, "SQL error: %s\n", zErrMsg);
    sqlite3_free (zErrMsg);
  }
  while (sqlite3_step (stmt) == SQLITE_ROW) {
    int id = sqlite3_column_int (stmt, 0);
    const unsigned char* name = sqlite3_column_text (stmt, 1);
    task = g_object_new (SHMOGGL_TYPE_TASK, "id", id, "name", name, NULL);
  }
  sqlite3_finalize (stmt);
  return task;
}

void
shmoggl_task_find (GPtrArray* tasks)
{
  char *zErrMsg = 0;
  int rc;
  gchar *sql = "SELECT * FROM tasks ORDER BY id DESC;";
  sqlite3_stmt *stmt;
  rc = sqlite3_prepare_v2 (shmoggl_database, sql, -1, &stmt, NULL);
  if (rc != SQLITE_OK) {
    fprintf (stderr, "SQL error: %s\n", zErrMsg);
    sqlite3_free (zErrMsg);
  }
  while (sqlite3_step (stmt) == SQLITE_ROW) {
    int id = sqlite3_column_int (stmt, 0);
    const unsigned char* name = sqlite3_column_text (stmt, 1);
    ShmogglTask *task = g_object_new(SHMOGGL_TYPE_TASK, "id", id, "name", name, NULL);
    g_ptr_array_add(tasks, task);
  }
  sqlite3_finalize (stmt);
}

void
shmoggl_task_start_time_tracking_thread (ShmogglTask *task)
{
  g_assert (SHMOGGL_IS_TASK (task));
  g_signal_emit (task, obj_signals[TASK_TRACKING_STARTED], 0);
  ShmogglTimeSlice* time_slice = shmoggl_time_slice_create (task);
  g_ptr_array_add (task->time_slices, time_slice);
  while (TRUE) {
    // Exit thread if tracking is stopped
    gboolean is_tracked = FALSE;
    g_object_get (task, "is_tracked", &is_tracked, NULL);
    if (!is_tracked) {
      g_thread_exit (NULL);
    }
    // Obtain current time
    GTimeVal current_time = { NULL };
    g_get_current_time (&current_time);
    // Update time slice
    g_object_set (time_slice, "stop", current_time.tv_sec, NULL);
    // Save changes
    shmoggl_time_slice_save (time_slice);
    // Wait for some time
    g_usleep (G_USEC_PER_SEC * 2);
  }
}

void
shmoggl_task_start_time_tracking (ShmogglTask *task)
{
  gboolean is_tracked = FALSE;
  g_object_get (task, "is_tracked", &is_tracked, NULL);
  if (is_tracked == FALSE) {
    g_object_set (task, "is_tracked", TRUE, NULL);
    GThread *thread = g_thread_new ("tracking_thread", (GThreadFunc)shmoggl_task_start_time_tracking_thread, task);
  }
}

void
shmoggl_task_stop_time_tracking (ShmogglTask *task)
{
  g_object_set (task, "is_tracked", FALSE, NULL);
  g_signal_emit (task, obj_signals[TASK_TRACKING_STOPPED], 0);
}

void
shmoggl_task_add_time_slice (ShmogglTask *task, ShmogglTimeSlice *time_slice)
{
  g_ptr_array_add (task->time_slices, time_slice);
}

void
shmoggl_task_delete (guint id)
{
  char *zErrMsg = 0;
  int rc;
  char sql[256];
  snprintf (sql, sizeof(sql), "DELETE FROM tasks WHERE id = %d;", id);
  rc = sqlite3_exec (shmoggl_database, sql, NULL, 0, &zErrMsg);
  if (rc != SQLITE_OK) {
    fprintf (stderr, "SQL error: %s\n", zErrMsg);
    sqlite3_free (zErrMsg);
  }
}

void
shmoggl_task_update (ShmogglTask *task)
{
  guint task_id;
  const gchar *task_name;
  g_object_get (task, "id", &task_id, "name", &task_name, NULL);
  char *zErrMsg = 0;
  int rc;
  char sql[256];
  snprintf (sql, sizeof(sql), "UPDATE tasks SET name = '%s' WHERE id = %d;", task_name, task_id);
  sqlite3_stmt *stmt;
  rc = sqlite3_prepare_v2 (shmoggl_database, sql, -1, &stmt, NULL);
  if (rc != SQLITE_OK) {
    fprintf (stderr, "SQL error: %s\n", zErrMsg);
    sqlite3_free (zErrMsg);
  }
  while (sqlite3_step (stmt) == SQLITE_ROW) {
    //
  }
  sqlite3_finalize (stmt);
  g_signal_emit (task, obj_signals[TASK_UPDATED], 0);
}

G_DEFINE_TYPE (ShmogglTask, shmoggl_task, G_TYPE_OBJECT)
