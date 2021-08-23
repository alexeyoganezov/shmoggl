/* shmoggl-time-slice-view.c
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

#include "shmoggl-time-slice-view.h"

struct _ShmogglTimeSliceView
{
  GtkBox            parent_instance;
  GtkLabel         *label_slice_name;
  GtkLabel         *label_slice_time;
  ShmogglTimeSlice *time_slice;
  GtkButton        *btn_delete_slice;
  GtkPopover       *popover_menu;
};

typedef enum
{
  PROP_TIME_SLICE = 1,
  N_PROPERTIES
} ShmogglTimeSliceViewProperty;

static GParamSpec *obj_properties[N_PROPERTIES] = { NULL, };

static void
shmoggl_time_slice_view_set_property (GObject      *object,
                                      guint         property_id,
                                      const GValue *value,
                                      GParamSpec   *pspec)
{
  ShmogglTimeSliceView *self = SHMOGGL_TIME_SLICE_VIEW (object);
  switch ((ShmogglTimeSliceViewProperty) property_id)
    {
    case PROP_TIME_SLICE:
      self->time_slice = g_value_get_pointer (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
shmoggl_time_slice_view_get_property (GObject    *object,
                                      guint       property_id,
                                      GValue     *value,
                                      GParamSpec *pspec)
{
  ShmogglTimeSliceView *self = SHMOGGL_TIME_SLICE_VIEW (object);
  switch ((ShmogglTimeSliceViewProperty) property_id)
    {
    case PROP_TIME_SLICE:
      g_value_set_pointer (value, self->time_slice);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
shmoggl_time_slice_view_class_init (ShmogglTimeSliceViewClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  gtk_widget_class_set_template_from_resource (widget_class, "/com/github/alexeyoganezov/shmoggl/shmoggl-time-slice-view.ui");
  gtk_widget_class_bind_template_child (widget_class, ShmogglTimeSliceView, label_slice_name);
  gtk_widget_class_bind_template_child (widget_class, ShmogglTimeSliceView, label_slice_time);
  gtk_widget_class_bind_template_child (widget_class, ShmogglTimeSliceView, btn_delete_slice);
  gtk_widget_class_bind_template_child (widget_class, ShmogglTimeSliceView, popover_menu);
  object_class->set_property = shmoggl_time_slice_view_set_property;
  object_class->get_property = shmoggl_time_slice_view_get_property;
  obj_properties[PROP_TIME_SLICE] =
    g_param_spec_pointer ("time-slice",
                          "",
                          "",
                          G_PARAM_READWRITE);
  g_object_class_install_properties (object_class,
                                     N_PROPERTIES,
                                     obj_properties);
}

void
shmoggl_time_slice_view_on_delete (GtkButton *button, ShmogglTimeSliceView *self)
{
  gtk_popover_popdown (self->popover_menu);
  ShmogglTimeSlice *time_slice;
  g_object_get (self, "time_slice", &time_slice, NULL);
  guint time_slice_id;
  g_object_get (time_slice, "id", &time_slice_id, NULL);
  shmoggl_time_slice_delete (time_slice_id);
  gtk_widget_destroy (GTK_WIDGET (self));
}

static void
shmoggl_time_slice_view_init (ShmogglTimeSliceView *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
  g_signal_connect (self->btn_delete_slice,
                    "clicked",
                    G_CALLBACK (shmoggl_time_slice_view_on_delete),
                    self);
}

void
shmoggl_time_slice_view_set_name (ShmogglTimeSliceView *self, const gchar *name)
{
  gtk_label_set_text (self->label_slice_name, name);
}

void
shmoggl_time_slice_view_set_duration (ShmogglTimeSliceView *self, const gchar *duration)
{
  gtk_label_set_text (self->label_slice_time, duration);
}

G_DEFINE_TYPE (ShmogglTimeSliceView, shmoggl_time_slice_view, GTK_TYPE_BOX)
