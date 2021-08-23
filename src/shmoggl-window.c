/* shmoggl-window.c
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

#include "shmoggl-config.h"
#include "shmoggl-window.h"

struct _ShmogglWindow
{
  GtkApplicationWindow   parent_instance;
  GtkButton             *btn_task_state_toggle;
  GtkImage              *img_task_state_toggle;
  GtkEntry              *entry_task_name;
  GtkLabel              *label_tracked_time;
  GtkListBox            *list_tasks;
  GPtrArray             *tasks;
  GPtrArray             *time_slices;
  GtkBox                *empty_view_wrapper;
  GtkScrolledWindow     *task_list;
  gboolean               is_tracking;
};

static void
shmoggl_window_class_init (ShmogglWindowClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  gtk_widget_class_set_template_from_resource (widget_class, "/com/github/alexeyoganezov/shmoggl/shmoggl-window.ui");
  gtk_widget_class_bind_template_child (widget_class, ShmogglWindow, btn_task_state_toggle);
  gtk_widget_class_bind_template_child (widget_class, ShmogglWindow, img_task_state_toggle);
  gtk_widget_class_bind_template_child (widget_class, ShmogglWindow, entry_task_name);
  gtk_widget_class_bind_template_child (widget_class, ShmogglWindow, list_tasks);
  gtk_widget_class_bind_template_child (widget_class, ShmogglWindow, label_tracked_time);
  gtk_widget_class_bind_template_child (widget_class, ShmogglWindow, empty_view_wrapper);
  gtk_widget_class_bind_template_child (widget_class, ShmogglWindow, task_list);
}

void
shmoggl_window_timer_updating_thread (ShmogglWindow *window)
{
  g_assert (SHMOGGL_IS_WINDOW (window));
  GTimeVal start = { NULL };
  g_get_current_time (&start);
  while (TRUE) {
    if (window->is_tracking == TRUE) {
      GTimeVal now = { NULL };
      g_get_current_time (&now);
      glong difference = now.tv_sec - start.tv_sec;
      gchar timer[16];
      shmoggl_utils_format_duration (timer, difference);
      gtk_label_set_text (window->label_tracked_time, timer);
    } else {
      g_thread_exit (NULL);
    }
    g_usleep (G_USEC_PER_SEC);
  }
}

void
shmoggl_window_on_task_view_deletion (ShmogglTaskView *task_view, ShmogglWindow *window)
{
  g_assert (SHMOGGL_IS_TASK_VIEW (task_view));
  g_assert (SHMOGGL_IS_WINDOW (window));
  // Delete task from array
  ShmogglTask *deleted_task;
  g_object_get (task_view, "task", &deleted_task, NULL);
  g_ptr_array_remove (window->tasks, deleted_task);
  // If no tasks left display a message
  guint number_of_tasks = window->tasks->len;
  if (number_of_tasks == 0) {
    gtk_widget_hide (GTK_WIDGET (window->task_list));
    gtk_widget_show (GTK_WIDGET (window->empty_view_wrapper));
  }
}

void
shmoggl_window_on_tracking_started (ShmogglTask *task, ShmogglWindow *window)
{
  g_assert (SHMOGGL_IS_TASK (task));
  g_assert (SHMOGGL_IS_WINDOW (window));
  gtk_editable_set_editable (GTK_EDITABLE (window->entry_task_name), FALSE);
  gchar *task_name;
  g_object_get (task, "name", &task_name, NULL);
  gtk_entry_set_text (window->entry_task_name, task_name);
  gtk_image_set_from_icon_name (window->img_task_state_toggle, "media-playback-stop", GTK_ICON_SIZE_BUTTON);
  window->is_tracking = TRUE;
  GThread *thread = g_thread_new ("timer_updating_thread", (GThreadFunc) shmoggl_window_timer_updating_thread, window);
}

void
shmoggl_window_on_tracking_stopped (ShmogglTask *task, ShmogglWindow *window)
{
  g_assert (SHMOGGL_IS_TASK (task));
  g_assert (SHMOGGL_IS_WINDOW (window));
  gtk_editable_set_editable (GTK_EDITABLE(window->entry_task_name), TRUE);
  gtk_entry_set_text (window->entry_task_name, "");
  gtk_image_set_from_icon_name (window->img_task_state_toggle, "media-playback-start", GTK_ICON_SIZE_BUTTON);
  window->is_tracking = FALSE;
  gtk_label_set_text (window->label_tracked_time, "0:00:00");
}

static void
shmoggl_window_switch_task_state (GtkButton *button, ShmogglWindow *self)
{
  g_assert (GTK_IS_BUTTON (button));
  g_assert (SHMOGGL_IS_WINDOW (self));
  const gchar* task_name = gtk_entry_get_text (self->entry_task_name);
  guint16 task_name_length = gtk_entry_get_text_length (self->entry_task_name);
  if (task_name_length == 0) {
    return;
  }
  // Check if time tracking performed right now
  gboolean is_tracking = FALSE;
  guint len = self->tasks->len;
  for (guint i = 0; i < len; i += 1) {
    ShmogglTask *task = g_ptr_array_index(self->tasks, i);
    g_assert (SHMOGGL_IS_TASK (task));
    gboolean is_tracked = FALSE;
    g_object_get (task, "is_tracked", &is_tracked, NULL);
    if (is_tracked == TRUE) {
      is_tracking = TRUE;
      break;
    }
  }
  if (is_tracking == FALSE) {
    // Check if a task with this name exists
    gboolean task_already_exists = FALSE;
    for (guint i = 0; i < len; i += 1) {
      ShmogglTask *task = g_ptr_array_index (self->tasks, i);
      g_assert (SHMOGGL_IS_TASK (task));
      gchar *name;
      g_object_get (task, "name", &name, NULL);
      if (g_strcmp0 (name, task_name) == 0) {
        task_already_exists = TRUE;
        break;
      }
    }
    if (task_already_exists == FALSE) {
      ShmogglTask *task = shmoggl_task_create (task_name);
      g_assert (SHMOGGL_IS_TASK (task));
      g_signal_connect (task, "tracking_started", (GCallback) shmoggl_window_on_tracking_started, self);
      g_signal_connect (task, "tracking_stopped", (GCallback) shmoggl_window_on_tracking_stopped, self);
      g_ptr_array_add (self->tasks, task);
      gchar *task_name;
      g_object_get (task, "name", &task_name, NULL);
      // TODO: fix duplication
      ShmogglTaskView *task_view = g_object_new (SHMOGGL_TYPE_TASK_VIEW, "task", task, NULL);
      g_signal_connect (task_view, "task_view_deleted", (GCallback) shmoggl_window_on_task_view_deletion, self);
      GtkWidget *task_view_widget = GTK_WIDGET (task_view);
      shmoggl_task_view_set_name (task_view, task_name);
      gtk_container_add (GTK_CONTAINER(self->list_tasks), task_view_widget);
      gtk_widget_show (GTK_WIDGET(self->task_list));
      gtk_widget_hide (GTK_WIDGET(self->empty_view_wrapper));
      shmoggl_task_start_time_tracking (task);
    } else {
      // Get task and start time tracking
      // TODO: do not reiterate over an array, get a task in a first place
      ShmogglTask *existing_task = NULL;
      for (guint i = 0; i < self->tasks->len; i += 1) {
        ShmogglTask *task = g_ptr_array_index(self->tasks, i);
        gchar *name;
        g_object_get (task, "name", &name, NULL);
        g_assert (SHMOGGL_IS_TASK(task));
        if (g_strcmp0 (name, task_name) == 0) {
          existing_task = task;
          break;
        }
      }
      shmoggl_task_start_time_tracking (existing_task);
    }
  } else {
    // Stop current time tracking
    // TODO: do not reiterate over an array, get a task in a first place
    for (guint i = 0; i < self->tasks->len; i += 1) {
      ShmogglTask *task = g_ptr_array_index (self->tasks, i);
      g_assert (SHMOGGL_IS_TASK (task));
      gboolean is_tracked = FALSE;
      g_object_get (task, "is_tracked", &is_tracked, NULL);
      if (is_tracked) {
        shmoggl_task_stop_time_tracking (task);
      }
    }
  }
}

static void
shmoggl_window_init (ShmogglWindow *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
  g_signal_connect (self->btn_task_state_toggle,
                    "clicked",
                    G_CALLBACK (shmoggl_window_switch_task_state),
                    self);
  // Load tasks
  self->tasks = g_ptr_array_new();
  shmoggl_task_find(self->tasks);
  guint number_of_tasks = self->tasks->len;
  // Load time slices
  self->time_slices = g_ptr_array_new();
  shmoggl_time_slice_find(self->time_slices);
  guint number_of_time_slices = self->time_slices->len;
  // Fill tasks with time slices
  // TODO: optimize algorithm
  for (guint i = 0; i < number_of_time_slices; i += 1) {
    // Get time slice
    ShmogglTimeSlice *time_slice = g_ptr_array_index (self->time_slices, i);
    g_assert (SHMOGGL_IS_TIME_SLICE (time_slice));
    // Get populated task id
    guint populated_task_id;
    g_object_get (time_slice, "task_id", &populated_task_id, NULL);
    // Get task
    ShmogglTask *task = NULL;
    guint number_of_tasks = self->tasks->len;
    for (guint j = 0; j < number_of_tasks; j += 1) {
      ShmogglTask *t = g_ptr_array_index(self->tasks, j);
      guint t_id;
      g_object_get (t, "id", &t_id, NULL);
      if (t_id == populated_task_id) {
        task = t;
        break;
      }
    }
    // Add a slice to it
    if (task != NULL) {
      shmoggl_task_add_time_slice (task, time_slice);
    }
  }
  // Render tasks or "no tasks" message
  if (number_of_tasks > 0) {
    for (guint i = 0; i < self->tasks->len; i += 1) {
      ShmogglTask *task = g_ptr_array_index (self->tasks, i);
      g_assert (SHMOGGL_IS_TASK (task));
      g_signal_connect (task, "tracking_started", (GCallback) shmoggl_window_on_tracking_started, self);
      g_signal_connect (task, "tracking_stopped", (GCallback) shmoggl_window_on_tracking_stopped, self);
      gchar *task_name;
      g_object_get(task, "name", &task_name, NULL);
      ShmogglTaskView *task_view = g_object_new (SHMOGGL_TYPE_TASK_VIEW, "task", task, NULL);
      g_signal_connect (task_view, "task_view_deleted", (GCallback) shmoggl_window_on_task_view_deletion, self);
      GtkWidget *task_view_widget = GTK_WIDGET (task_view);
      shmoggl_task_view_set_name (task_view, task_name);
      gtk_container_add (GTK_CONTAINER (self->list_tasks), task_view_widget);
    }
    gtk_widget_show_all (GTK_WIDGET (self->list_tasks));
  } else {
    // TODO: move it to a class method
    gtk_widget_hide(GTK_WIDGET(self->task_list));
    gtk_widget_show(GTK_WIDGET(self->empty_view_wrapper));
  }
  // Setup task name autocompletion
  GtkListStore *store;
  GtkTreeIter iter;
  store = gtk_list_store_new (1, G_TYPE_STRING);
  for (guint i = 0; i < number_of_tasks; i++)
    {
      ShmogglTask *task = g_ptr_array_index (self->tasks, i);
      gchar *task_name;
      g_object_get (task, "name", &task_name, NULL);
      gtk_list_store_append (store, &iter);
      gtk_list_store_set (store, &iter, 0, task_name, -1);
    }
  GtkEntryCompletion *completion = gtk_entry_completion_new ();
  gtk_entry_set_completion (GTK_ENTRY (self->entry_task_name), completion);
  g_object_unref (completion);
  gtk_entry_completion_set_model (completion, GTK_TREE_MODEL (store));
  g_object_unref (store);
  gtk_entry_completion_set_text_column (completion, 0);
}

G_DEFINE_TYPE (ShmogglWindow, shmoggl_window, GTK_TYPE_APPLICATION_WINDOW)
