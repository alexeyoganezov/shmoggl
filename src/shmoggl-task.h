#pragma once

#include <gtk/gtk.h>
#include "shmoggl-time-slice.h"
#include "shmoggl-database.h"
#include "dependencies/sqlite/sqlite3.h"

G_BEGIN_DECLS

#define SHMOGGL_TYPE_TASK (shmoggl_task_get_type())
G_DECLARE_FINAL_TYPE (ShmogglTask, shmoggl_task, SHMOGGL, TASK, GObject)

ShmogglTask * shmoggl_task_create(const gchar* taskName);
void shmoggl_task_find(GPtrArray* tasks);
void shmoggl_task_start_time_tracking(ShmogglTask *task);
void shmoggl_task_stop_time_tracking(ShmogglTask *task);
void shmoggl_task_add_time_slice(ShmogglTask *task, ShmogglTimeSlice *time_slice);
void shmoggl_task_delete(guint id);
void shmoggl_task_update(ShmogglTask *task);

G_END_DECLS
