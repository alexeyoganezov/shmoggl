#pragma once

#include <glib-object.h>
#include <gtk/gtk.h>
#include "shmoggl-time-slice.h"

G_BEGIN_DECLS

#define SHMOGGL_TYPE_TIME_SLICE_VIEW shmoggl_time_slice_view_get_type ()
G_DECLARE_FINAL_TYPE (ShmogglTimeSliceView, shmoggl_time_slice_view, SHMOGGL, TIME_SLICE_VIEW, GtkBox)

void shmoggl_time_slice_view_set_name (ShmogglTimeSliceView *self, const gchar *name);
void shmoggl_time_slice_view_set_duration (ShmogglTimeSliceView *self, const gchar *duration);

G_END_DECLS
