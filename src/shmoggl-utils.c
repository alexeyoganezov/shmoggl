/* shmoggl-utils.c
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

#include "shmoggl-utils.h"

void
shmoggl_utils_format_duration (gchar *buffer, glong duration)
{
  glong hours = (duration / (60 * 60) % 24);
  glong minutes = (duration / 60) % 60;
  glong seconds = (duration % 60);
  // Add leding zero to seconds
  gchar strSeconds[3];
  if (seconds < 10) {
    sprintf (strSeconds, "0%ld", seconds);
  } else {
    sprintf (strSeconds, "%ld", seconds);
  }
  // Add leding zero to minutes
  gchar strMinutes[3];
  if (minutes < 10) {
    sprintf (strMinutes, "0%ld", minutes);
  } else {
    sprintf (strMinutes, "%ld", minutes);
  }
  sprintf (buffer, "%ld:%s:%s", hours, strMinutes, strSeconds);
}
