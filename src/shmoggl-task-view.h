#pragma once

#include <glib-object.h>
#include <gtk/gtk.h>
#include "shmoggl-task.h"
#include "shmoggl-time-slice.h"
#include "shmoggl-time-slice-view.h"
#include "shmoggl-utils.h"

G_BEGIN_DECLS

#define SHMOGGL_TYPE_TASK_VIEW shmoggl_task_view_get_type ()
G_DECLARE_FINAL_TYPE (ShmogglTaskView, shmoggl_task_view, SHMOGGL, TASK_VIEW, GtkBox)

void shmoggl_task_view_set_name (ShmogglTaskView *self, const gchar *name);

G_END_DECLS
