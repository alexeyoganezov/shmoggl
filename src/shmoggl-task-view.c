/* shmoggl-task-view.c
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

#include "shmoggl-task-view.h"

struct _ShmogglTaskView
{
  GtkBox       parent_instance;
  GtkLabel    *label_name;
  GtkLabel    *label_time;
  GtkButton   *btn_task_state_toggle;
  ShmogglTask *task;
  GThread     *tracking_thread;
  GtkListBox  *list_slices;
  GtkButton   *btn_expand;
  GtkButton   *btn_delete;
  GtkButton   *btn_edit;
  GtkRevealer *revealer_slices;
  guint        tracked_total;
  GtkPopover  *popover_menu;
  GtkDialog   *dialog_task_edit;
  GtkEntry    *entry_task_name;
  GtkButton   *btn_save_task;
  GtkImage    *img_task_state_toggle;
};

G_DEFINE_TYPE (ShmogglTaskView, shmoggl_task_view, GTK_TYPE_BOX)

typedef enum
{
  PROP_TASK = 1,
  N_PROPERTIES
} ShmogglTaskViewProperty;

static GParamSpec *obj_properties[N_PROPERTIES] = { NULL, };

typedef enum
{
  TASK_VIEW_DELETED,
  N_SIGNALS
} ShmogglTaskViewSignal;

static guint obj_signals[N_SIGNALS] = { 0 };

void
shmoggl_task_view_on_tracking_started (ShmogglTask *task, ShmogglTaskView *task_view)
{
  g_assert (SHMOGGL_IS_TASK (task));
  g_assert (SHMOGGL_IS_TASK_VIEW (task_view));
  gtk_image_set_from_icon_name (task_view->img_task_state_toggle, "media-playback-stop", GTK_ICON_SIZE_BUTTON);
}

void
shmoggl_task_view_on_tracking_stopped (ShmogglTask *task, ShmogglTaskView *task_view)
{
  g_assert (SHMOGGL_IS_TASK (task));
  g_assert (SHMOGGL_IS_TASK_VIEW (task_view));
  gtk_image_set_from_icon_name (task_view->img_task_state_toggle, "media-playback-start", GTK_ICON_SIZE_BUTTON);
  // Get last time slice
  GPtrArray *time_slices;
  g_object_get (task, "time_slices", &time_slices, NULL);
  guint number_of_slices = time_slices->len;
  ShmogglTimeSlice *time_slice = g_ptr_array_index (time_slices, number_of_slices - 1);
  // Update total time tracked
  glong time_slice_start;
  glong time_slice_stop;
  g_object_get (time_slice, "start", &time_slice_start, "stop", &time_slice_stop, NULL);
  gchar timer[16];
  guint slice_duration = time_slice_stop - time_slice_start;
  task_view->tracked_total += slice_duration;
  shmoggl_utils_format_duration (timer, task_view->tracked_total);
  gtk_label_set_text (task_view->label_time, timer);
  // Render new time slice
  ShmogglTimeSliceView *time_slice_view = g_object_new (SHMOGGL_TYPE_TIME_SLICE_VIEW, "time_slice", time_slice, NULL);
  GtkWidget *time_slice_view_widget = GTK_WIDGET (time_slice_view);
  gchar period[256];
  GDateTime *start = g_date_time_new_from_unix_utc (time_slice_start);
  gchar *start_string = g_date_time_format (start, "%R %d.%m.%Y");
  GDateTime *stop = g_date_time_new_from_unix_utc (time_slice_stop);
  gchar *stop_string = g_date_time_format (stop, "%R %d.%m.%Y");
  sprintf (period, "%s - %s", start_string, stop_string);
  shmoggl_time_slice_view_set_name (time_slice_view, period);
  gchar duration_string[16];
  shmoggl_utils_format_duration (duration_string, slice_duration);
  shmoggl_time_slice_view_set_duration (time_slice_view, duration_string);
  gtk_list_box_prepend (task_view->list_slices, time_slice_view_widget);
}

static void
shmoggl_task_view_set_property (GObject      *object,
                                guint         property_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  ShmogglTaskView *self = SHMOGGL_TASK_VIEW (object);
  switch ((ShmogglTaskViewProperty) property_id)
    {
    case PROP_TASK:
      self->task = g_value_get_pointer (value);
      g_signal_connect (self->task, "tracking_started", (GCallback) shmoggl_task_view_on_tracking_started, self);
      g_signal_connect (self->task, "tracking_stopped", (GCallback) shmoggl_task_view_on_tracking_stopped, self);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
shmoggl_task_view_get_property (GObject    *object,
                                guint       property_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  ShmogglTaskView *self = SHMOGGL_TASK_VIEW (object);
  switch ((ShmogglTaskViewProperty) property_id)
    {
    case PROP_TASK:
      g_value_set_pointer (value, self->task);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

void
shmoggl_task_view_on_button_click (GtkButton *button, ShmogglTaskView *self)
{
  g_assert (GTK_IS_BUTTON (button));
  g_assert (SHMOGGL_IS_TASK_VIEW (self));
  ShmogglTask *task;
  g_object_get (self, "task", &task, NULL);
  gboolean is_tracked = FALSE;
  g_object_get(task, "is_tracked", &is_tracked, NULL);
  // TODO: move this logic to signal handler
  if (is_tracked == FALSE) {
    shmoggl_task_start_time_tracking (task);
    gtk_button_set_label (self->btn_task_state_toggle, "Stop");
  } else {
    shmoggl_task_stop_time_tracking (task);
    gtk_button_set_label (self->btn_task_state_toggle, "Start");
  }
}

void
shmoggl_task_view_on_task_update (ShmogglTask *task, ShmogglTaskView *self)
{
  gchar *task_name = NULL;
  g_object_get (task, "name", &task_name, NULL);
  shmoggl_task_view_set_name (self, task_name);
}

static void
shmoggl_task_view_constructed (ShmogglTaskView *self)
{
  GPtrArray *time_slices = NULL;
  g_assert (SHMOGGL_IS_TASK (self->task));
  g_object_get (self->task, "time_slices", &time_slices, NULL);
  guint number_of_time_slices = time_slices->len;
  g_signal_connect (self->task, "task_updated", (GCallback) shmoggl_task_view_on_task_update, self);
  // Render time slices
  for (guint i = 0; i < number_of_time_slices; i += 1) {
    ShmogglTimeSlice *time_slice = g_ptr_array_index (time_slices, i);
    g_assert (SHMOGGL_IS_TIME_SLICE (time_slice));
    glong time_slice_start;
    glong time_slice_stop;
    g_object_get (time_slice, "start", &time_slice_start, "stop", &time_slice_stop, NULL);
    // Create and add element
    ShmogglTimeSliceView *time_slice_view = g_object_new (SHMOGGL_TYPE_TIME_SLICE_VIEW, "time_slice", time_slice, NULL);
    GtkWidget *time_slice_view_widget = GTK_WIDGET (time_slice_view);
    // Dislay slice start and stop time
    // TODO: fix duplication with above, move this logic to slice view (use above version)
    gchar period[256];
    GDateTime *start = g_date_time_new_from_unix_utc (time_slice_start);
    gchar *start_string = g_date_time_format (start, "%R %d.%m.%Y");
    GDateTime *stop = g_date_time_new_from_unix_utc (time_slice_stop);
    gchar *stop_string = g_date_time_format (stop, "%R %d.%m.%Y");
    sprintf (period, "%s - %s", start_string, stop_string);
    shmoggl_time_slice_view_set_name (time_slice_view, period);
    gtk_container_add (GTK_CONTAINER (self->list_slices), time_slice_view_widget);
    // Display slice duration
    gchar timer[16];
    guint slice_duration = time_slice_stop - time_slice_start;
    shmoggl_utils_format_duration (timer, slice_duration);
    shmoggl_time_slice_view_set_duration (time_slice_view, timer);
    self->tracked_total += slice_duration;
  }
  // Display total time tracked
  gchar timer[16];
  shmoggl_utils_format_duration(timer, self->tracked_total);
  gtk_label_set_text (self->label_time, timer);
  //
  G_OBJECT_CLASS (shmoggl_task_view_parent_class)->constructed (G_OBJECT(self));
}

static void
shmoggl_task_view_class_init (ShmogglTaskViewClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  gtk_widget_class_set_template_from_resource (widget_class, "/com/github/alexeyoganezov/shmoggl/shmoggl-task-view.ui");
  gtk_widget_class_bind_template_child (widget_class, ShmogglTaskView, label_name);
  gtk_widget_class_bind_template_child (widget_class, ShmogglTaskView, label_time);
  gtk_widget_class_bind_template_child (widget_class, ShmogglTaskView, btn_task_state_toggle);
  gtk_widget_class_bind_template_child (widget_class, ShmogglTaskView, list_slices);
  gtk_widget_class_bind_template_child (widget_class, ShmogglTaskView, btn_expand);
  gtk_widget_class_bind_template_child (widget_class, ShmogglTaskView, btn_delete);
  gtk_widget_class_bind_template_child (widget_class, ShmogglTaskView, btn_edit);
  gtk_widget_class_bind_template_child (widget_class, ShmogglTaskView, revealer_slices);
  gtk_widget_class_bind_template_child (widget_class, ShmogglTaskView, popover_menu);
  gtk_widget_class_bind_template_child (widget_class, ShmogglTaskView, dialog_task_edit);
  gtk_widget_class_bind_template_child (widget_class, ShmogglTaskView, entry_task_name);
  gtk_widget_class_bind_template_child (widget_class, ShmogglTaskView, btn_save_task);
  gtk_widget_class_bind_template_child (widget_class, ShmogglTaskView, img_task_state_toggle);
  object_class->set_property = shmoggl_task_view_set_property;
  object_class->get_property = shmoggl_task_view_get_property;
  object_class->constructed = shmoggl_task_view_constructed;
  obj_properties[PROP_TASK] =
    g_param_spec_pointer ("task",
                          "Displayed task",
                          "",
                          G_PARAM_CONSTRUCT | G_PARAM_READWRITE);
  g_object_class_install_properties (object_class,
                                     N_PROPERTIES,
                                     obj_properties);
  obj_signals[TASK_VIEW_DELETED] =
    g_signal_new ("task_view_deleted",
                  G_OBJECT_CLASS_TYPE (object_class),
                  G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
                  0,
                  NULL,
                  NULL,
                  NULL,
                  G_TYPE_NONE,
                  0);
}

void
shmoggl_task_view_on_click (GtkButton *button, ShmogglTaskView *self)
{
  gboolean revealed = gtk_revealer_get_child_revealed (self->revealer_slices);
  gtk_revealer_set_reveal_child (self->revealer_slices, !revealed);
}

void
shmoggl_task_view_on_delete (GtkButton *button, ShmogglTaskView *self)
{
  gtk_popover_popdown (self->popover_menu);
  ShmogglTask *task;
  g_object_get (self, "task", &task, NULL);
  guint task_id;
  g_object_get (task, "id", &task_id, NULL);
  g_signal_emit (self, obj_signals[TASK_VIEW_DELETED], 0);
  shmoggl_task_delete (task_id);
  gtk_widget_destroy (self);
}

void
shmoggl_task_view_on_edit (GtkButton *button, ShmogglTaskView *self)
{
  gtk_popover_popdown (self->popover_menu);
  ShmogglTask *task;
  g_object_get (self, "task", &task, NULL);
  guint task_id;
  g_object_get (task, "id", &task_id, NULL);
  const gchar *task_name;
  g_object_get (task, "name", &task_name, NULL);
  gtk_entry_set_text (self->entry_task_name, task_name);
  gtk_widget_show (self->dialog_task_edit);
}

void
shmoggl_task_view_on_save_task (GtkButton *button, ShmogglTaskView *self)
{
  ShmogglTask *task;
  g_object_get (self, "task", &task, NULL);
  const gchar* task_name = gtk_entry_get_text (self->entry_task_name);
  g_object_set (task, "name", task_name, NULL);
  shmoggl_task_update (task);
  gtk_widget_hide (GTK_WIDGET (self->dialog_task_edit));
}

static void
shmoggl_task_view_init (ShmogglTaskView *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
  g_signal_connect (self->btn_task_state_toggle,
                    "clicked",
                    G_CALLBACK (shmoggl_task_view_on_button_click),
                    self);
  g_signal_connect (self->btn_expand,
                    "clicked",
                    G_CALLBACK (shmoggl_task_view_on_click),
                    self);
  g_signal_connect (self->btn_delete,
                    "clicked",
                    G_CALLBACK (shmoggl_task_view_on_delete),
                    self);
  g_signal_connect (self->btn_edit,
                    "clicked",
                    G_CALLBACK (shmoggl_task_view_on_edit),
                    self);
  g_signal_connect (self->btn_save_task,
                    "clicked",
                    G_CALLBACK (shmoggl_task_view_on_save_task),
                    self);
  self->tracking_thread = NULL;
  self->tracked_total = 0;
}

void
shmoggl_task_view_set_name (ShmogglTaskView *self, const gchar *name)
{
  gtk_label_set_text (self->label_name, name);
}
