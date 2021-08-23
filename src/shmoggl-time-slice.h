#pragma once

#include <glib.h>
#include "shmoggl-database.h"
#include "dependencies/sqlite/sqlite3.h"

G_BEGIN_DECLS

#define SHMOGGL_TYPE_TIME_SLICE (shmoggl_time_slice_get_type())
G_DECLARE_FINAL_TYPE (ShmogglTimeSlice, shmoggl_time_slice, SHMOGGL, TIME_SLICE, GObject)

ShmogglTimeSlice* shmoggl_time_slice_create(gpointer *task);
void shmoggl_time_slice_save(ShmogglTimeSlice *time_slice);
void shmoggl_time_slice_find(GPtrArray* time_slices);
void shmoggl_time_slice_delete(guint id);

G_END_DECLS
