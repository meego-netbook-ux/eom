/* eom: sample picture view application (Eye of Moblin)
 *
 * Copyright (C) 2009 Intel Corp.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * Written by: Roger WANG <roger.wang@intel.com>
 */

#include "config.h"
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>
#include <gio/gio.h>
#include <clutter/clutter.h>
#include <clutter/x11/clutter-x11.h>

#include <tidy/tidy-viewport.h>
#include <tidy/tidy-depth-group.h>
#include <tidy/tidy-dark-texture.h>

#include "debug.h"

#define FPS 60

#ifdef HAVE_GESTURE
#include <clutter-gesture/clutter-gesture.h>
#endif

#define RECT_W 400
#define RECT_H 400
#define RECT_N 20
#define RECT_GAP 50
#define FRAMES 20

static gint stage_width, stage_height;
static ClutterTimeline *timeline;
static gint start, target;
static gboolean pressed = FALSE;
static gfloat pressed_x, pressed_y, pressed_viewport_x, pressed_viewport_y, pressed_device;
static ClutterActor* pic_group, *single_pic, *current_actor, *single_view_bg, *g_viewport;
static gfloat single_view_x0, single_view_y0;
static GList* pic_actors;
static gint total_pics;
static const char* folder;

static gboolean do_rotation_timeline = FALSE;
static gboolean test_mode = FALSE;
static ClutterTimeline* rotation_timeline;

static int rotate_test_fps, rotate_test_angle_delta;

static int g_speed = 30;
static  gboolean pinch_only = FALSE, rotate_only = FALSE;

gboolean pinch_center_set = FALSE;
gfloat pinch_x, pinch_y;

gboolean rotation_center_set = FALSE;
gfloat rot_x, rot_y;

void switch_to_single_view (ClutterGroup* group);
void single_view_navigate (gboolean next);
gboolean is_in_single_view_mode();
gboolean zoom_at_point (int x, int y, float scale);
void slide_viewport (TidyViewport* viewport, gint old_target, gint new_target, gint frames);
static void construct_stage();

static void
rotate_single_pic (int fps, int angle_delta)
{
  int total_time = 360 * 1000 / angle_delta / fps;
  gfloat width, height;

  clutter_actor_get_size (single_pic, &width, &height);
  clutter_actor_set_rotation (single_pic, CLUTTER_Z_AXIS, 0,
                              width / 2, height / 2, 0);
  clutter_actor_animate (single_pic, CLUTTER_LINEAR, total_time,
                         "rotation-angle-z", 359.9,
                         NULL);
}

#ifdef HAVE_GESTURE
static void start_rotate_viewport (TidyViewport* viewport, gboolean right);

static gboolean
gesture_pinch_cb (ClutterGesture    *gesture,
                  ClutterGesturePinchEvent *event,
                  gpointer         data)
{
  gdouble scale_x0, scale_y0, scale;
  gfloat x_start_1, y_start_1, x_start_2, y_start_2, x_end_1, y_end_1, x_end_2, y_end_2;

  if (!is_in_single_view_mode())
    return FALSE;

  x_start_1 = event->x_start_1;
  y_start_1 = event->y_start_1;
  x_start_2 = event->x_start_2;
  y_start_2 = event->y_start_2;
  x_end_1 = event->x_end_1;
  y_end_1 = event->y_end_1;
  x_end_2 = event->x_end_2;
  y_end_2 = event->y_end_2;

  DBG ("----> pinch: start event = (%lf,%lf) - (%lf,%lf)\n",
          x_start_1, y_start_1, x_start_2, y_start_2);

  DBG ("----> pinch:   end event = (%lf,%lf) - (%lf,%lf)\n",
          x_end_1, y_end_1, x_end_2, y_end_2);

  gdouble dist_start = hypot(((gdouble)x_start_2 - x_start_1),
               ((gdouble)y_start_2 - y_start_1));
  gdouble dist_end = hypot(((gdouble)x_end_2 - x_end_1),
               ((gdouble)y_end_2 - y_end_1));
  scale = dist_end / dist_start;
  clutter_actor_get_scale (single_pic, &scale_x0, &scale_y0);

  if (!pinch_center_set)
    {
      /* we are in the first pinch event */
      pinch_x = (x_start_1 + x_start_2) / 2;
      pinch_y = (y_start_1 + y_start_2) / 2;
      pinch_center_set = TRUE;
    }

  DBG ("----> pinch: scale center = (%lf,%lf) [%lf,%lf]\n",
       pinch_x, pinch_y, clutter_actor_get_width(single_pic),
       clutter_actor_get_height (single_pic));

  DBG ("----> pinch: scale = %lf,%lf\n", scale * scale_x0,
       scale * scale_y0);

  clutter_actor_set_scale_full (single_pic, scale * scale_x0, scale * scale_y0,
                                pinch_x, pinch_y);

  /* we don't want motion event while we're pinching */
  pressed = FALSE;

  return TRUE;
}


static void
intersection (int x1, int y1, int x2, int y2, int x3, int y3, int x4, int y4,
              int* x0, int* y0, int* angle)
{
  /* equation: a * x + b * y + c = 0 */

  /* we first calculate a, b, c for line (x1, y1)--(x2, y2) */
  int a1 = y2 - y1;
  int b1 = x1 - x2;
  int c1 = x2 * y1 - x1 * y2;

  /* now the second line */
  int a2 = y4 - y3;
  int b2 = x3 - x4;
  int c2 = x4 * y3 - x3 * y4;

  if (b2 * a1 - b1 * a2 == 0)
    {
      *x0 = x1;
      *y0 = y1;
      *angle = 0;
      return;
    }
  *x0 = (b1 * c2 - b2 * c1) / (b2 * a1 - b1 * a2);
  *y0 = (a1 * c2 - a2 * c1) / (a2 * b1 - a1 * b2);
  int angle1 = (atan2 (-a1, b1) / M_PI * 180);
  int angle2 = (atan2 (-a2, b2) / M_PI * 180);
  if (angle1 < 0)
    angle1 += 360;
  if (angle2 < 0)
    angle2 += 360;


  DBG ("angle1 = %d, angle2 = %d\n", angle1, angle2);

  if (abs(angle1 - angle2) > 180)
    {
      *angle = 360 - abs(angle1 - angle2);
      if (angle1 > angle2)
        *angle = -1 * (*angle);
    }
  else
    *angle = angle1 - angle2; /* clockwise */

  DBG ("return angle = %d\n", *angle);

}

static void
clear_rotation_timeline(ClutterTimeline *timeline,
                        gpointer clone)
{
  rotation_timeline = NULL;
}

static gboolean
gesture_rotate_cb (ClutterGesture    *gesture,
                   ClutterGestureRotateEvent    *event,
                   gpointer         data)
{
  int x0, y0;  /* rotation center */
  int angle0;
  gdouble angle, prev_angle;
  gfloat x, y, z;
  static ClutterAnimation* ani = NULL;

  if (!is_in_single_view_mode())
    return FALSE;

  pressed = FALSE;
  intersection (event->x_start_1, event->y_start_1,
                event->x_start_2, event->y_start_2,
                event->x_end_1,   event->y_end_1,
                event->x_end_2,   event->y_end_2,
                &x0, &y0, &angle0);
  angle = prev_angle = clutter_actor_get_rotation (single_pic, CLUTTER_Z_AXIS,
                                                   &x, &y, &z);
  angle -= angle0;

  if (!rotation_center_set)
    {
      rot_x = x0;
      rot_y = y0;
      rotation_center_set = TRUE;
    }

#ifdef HAVE_DEBUG
  DBG ("---> setting actor rotation: %d\n", (int)angle);
#endif

  if (do_rotation_timeline)
    {
      if (rotation_timeline)
        {
          clutter_animation_completed (ani);
        }
      /* setting rotation center only */
      clutter_actor_set_rotation (single_pic, CLUTTER_Z_AXIS, prev_angle,
                                rot_x, rot_y, z);
      ani = clutter_actor_animate (single_pic, CLUTTER_LINEAR, 60,
                                       "rotation-angle-z", angle,
                                       NULL);
      rotation_timeline = clutter_animation_get_timeline (ani);
      g_signal_connect (rotation_timeline, "completed",
                        G_CALLBACK(clear_rotation_timeline), NULL);
    }
  else
    clutter_actor_set_rotation (single_pic, CLUTTER_Z_AXIS, angle,
                                rot_x, rot_y, z);
  return TRUE;
}

#ifdef HAVE_TWO_FINGER_SLIDE
static gboolean
gesture_navigate_cb (ClutterGesture    *gesture,
                     ClutterGestureNavigateEvent    *event,
                     TidyViewport         *viewport)
{
  gint x, y, z;
  DBG ("gesture_navigate_cb is called\n");
  if (!is_in_single_view_mode())
    {
      DBG ("event->x_start_1 is %d, event->x_end_1 is %d\n", event->x_start_1, event->x_end_1);
      tidy_viewport_get_origin (TIDY_VIEWPORT (viewport), &x, &y, &z);
      tidy_viewport_set_origin (TIDY_VIEWPORT (viewport),
                                x + event->x_start_1 - event->x_end_1,
                                y, z);
    }

  return FALSE;
}
#endif

static gboolean
gesture_slide_cb (ClutterGesture    *gesture,
                  ClutterGestureEvent    *event,
                  gpointer         data)
{
  /* const char *gesture_name[] = {"dummy", */
  /*                               "SLIDE", "PINCH", "WINDOW", "ANY"}; */

  const char *slide_dir_name [] = {
    "dummy",
    "SLIDE_UP", "SLIDE_DOWN", "SLIDE_LEFT", "SLIDE_RIGHT"};

  DBG ("gesture_cb: event pointer %p\n", event);

  if (event && event->type == GESTURE_SLIDE)
    {
      ClutterGestureSlideEvent *slide = (ClutterGestureSlideEvent *)event;
      DBG ("slide direction :%s\n", slide_dir_name[slide->direction]);

      switch (slide->direction)
        {
        case SLIDE_DOWN:
          if (!is_in_single_view_mode())
            switch_to_single_view (CLUTTER_GROUP(pic_group));
          else
            {
              if (rotate_test_fps > 0)
                {
                  rotate_single_pic (rotate_test_fps, rotate_test_angle_delta);
                }
              else
                zoom_at_point (slide->x_start, slide->y_start, 0.8);
            }
          break;
        case SLIDE_LEFT:
        case SLIDE_RIGHT:
          if (is_in_single_view_mode())
            single_view_navigate (slide->direction == SLIDE_RIGHT);
          else {
            pressed = FALSE;
            start_rotate_viewport (TIDY_VIEWPORT(g_viewport), slide->direction == SLIDE_LEFT);
          }
          break;
        case SLIDE_UP:
          if (is_in_single_view_mode())
            zoom_at_point (slide->x_start, slide->y_start, 1.0/0.8);
          break;
      }
    }

  return TRUE;
}

#endif

static void
switch_single_pic_var(ClutterTimeline *timeline,
                      gpointer clone)
{
  gfloat x, y;

  clutter_actor_destroy (single_pic);
  single_pic = (ClutterActor*)clone;
  clutter_actor_get_position (clone, &x, &y);
  single_view_x0 = x;
  single_view_y0 = y;
}

static void
view_pic (ClutterActor* actor, gboolean from_right)
{
  gfloat width, height;
  gfloat new_view_x0, new_view_y0;
  ClutterColor test_block_color = { 0x7f, 0xae, 0xff, 0xff };

  ClutterActor* stage = clutter_stage_get_default();

  clutter_actor_get_size (actor, &width, &height);
  ClutterActor* clone;

  if (test_mode)
    clone = clutter_rectangle_new_with_color (&test_block_color);
  else
    {
#ifdef HAVE_GAMMA
      g_object_set (actor, "gamma", 255, NULL);
#endif
      clone = clutter_clone_new (actor);
    }

  gfloat disp_height = height * stage_width / width;
  gfloat disp_width = width * stage_height / height;

  if (disp_height > stage_height)
    {
      disp_height = stage_height;
      new_view_x0 = stage_width/2 - disp_width/2;
      new_view_y0 = 0;
    }
  else
    {
      disp_width = stage_width;
      new_view_x0 = 0;
      new_view_y0 = stage_height / 2 - disp_height/2;
    }

  clutter_actor_set_size (clone, disp_width, disp_height);
  clutter_container_add_actor (CLUTTER_CONTAINER (stage), clone);
  if (single_pic)
    {
      ClutterAnimation* ani_1, *ani_2;
      if (from_right)
        {
          ani_1 = clutter_actor_animate(single_pic, CLUTTER_LINEAR, 250,
                                        "x", -width, NULL);
          clutter_actor_set_position (clone, stage_width, new_view_y0);
        }
      else
        {
          ani_1 = clutter_actor_animate(single_pic, CLUTTER_LINEAR, 250,
                                        "x", stage_width, NULL);
          clutter_actor_set_position (clone, -disp_width, new_view_y0);
        }
      ClutterTimeline* timeline = clutter_animation_get_timeline (ani_1);
      g_signal_connect (timeline, "completed", G_CALLBACK(switch_single_pic_var), clone);
      ani_2 = clutter_actor_animate_with_timeline(clone, CLUTTER_LINEAR, timeline,
                                                  "x", new_view_x0, NULL);
    }
  else /* we just entered single view */
    {
      clutter_actor_set_position (clone, new_view_x0, new_view_y0);
      single_view_y0 = new_view_y0;
      single_view_x0 = new_view_x0;
      single_pic = clone;
    }
  clutter_actor_show (clone);

  current_actor = actor;
}

/* returns TRUE when we're already in original scale & angle
 * setting scale < 0 for resetting the view
 */

gboolean
zoom_at_point (int x, int y, float scale)
{
  ClutterGeometry geo;
  gdouble scale_x, scale_y, angle;
  gfloat x0, y0, z0;

  clutter_actor_get_geometry (single_pic, &geo);
  clutter_actor_get_scale (single_pic, &scale_x, &scale_y);

  if (scale < 0)
    {
      /* return to origin scale */
      angle = clutter_actor_get_rotation (single_pic, CLUTTER_Z_AXIS, &x0, &y0, &z0);
      if (geo.x == single_view_x0 && geo.y == single_view_y0
          && scale_x == 1 && scale_y == 1 && fabs(angle) < 0.1)
        return TRUE;

      clutter_actor_set_position (single_pic, single_view_x0, single_view_y0);
      clutter_actor_set_scale (single_pic, 1, 1);
      clutter_actor_set_rotation (single_pic, CLUTTER_Z_AXIS, 0, x0, y0, z0);
    }
  else
    {
      gfloat newx = geo.x - ( x - stage_width / 2 );
      gfloat newy = geo.y - ( y - stage_height / 2 );
      clutter_actor_animate (single_pic,
                             CLUTTER_LINEAR, 500,
                             "x", newx > 0 ? 0 : newx,
                             "y", newy > single_view_y0 ? single_view_y0 : newy,
                             "scale-x", scale_x * scale,
                             "scale-y", scale_y * scale,
                             NULL);
    }
  return FALSE;
}

void
single_view_navigate (gboolean next)
{
  GList* list = g_list_find (pic_actors, current_actor);
  if (next && list->next)
    view_pic ((ClutterActor*)list->next->data, FALSE);
  else if (!next && list->prev)
    view_pic ((ClutterActor*)list->prev->data, TRUE);
}

gboolean
is_in_single_view_mode()
{
  return single_pic != NULL;
}

void
exit_single_view()
{
  ClutterActor* stage = clutter_stage_get_default();
  clutter_actor_destroy (single_pic);
  clutter_actor_destroy (single_view_bg);

  g_object_notify (G_OBJECT (g_viewport), "x-origin");
  clutter_actor_show (g_viewport);

  single_pic = NULL;
}

void
switch_to_single_view (ClutterGroup* group)
{
  ClutterActor *actor;
  gfloat width, height;
  /* gint x, i; */
  /* ClutterActor* viewport = clutter_actor_get_parent (group); */
  /* tidy_viewport_get_origin (TIDY_VIEWPORT (viewport), &x, NULL, NULL); */
  /* GList* children = clutter_container_get_children (CLUTTER_CONTAINER(group)); */
  /* GList* c; */
  /* for (c = children, i = 0; c && i < x / RECT_GAP; c = c->next, i++); */
  /* if (!c) */
  /*   return; */
  /* actor = (ClutterActor *)c->data; */
  ClutterActor* stage = clutter_stage_get_default();
  clutter_actor_get_size (stage, &width, &height);
  actor = clutter_stage_get_actor_at_pos(CLUTTER_STAGE(stage), CLUTTER_PICK_ALL,
                                         width / 2, height / 2);

  ClutterColor bg_color = { 0x34, 0x39, 0x39, 0xff };
  ClutterActor* bg = clutter_rectangle_new_with_color (&bg_color);
  clutter_actor_set_size (bg, stage_width, stage_height);

  clutter_actor_hide (g_viewport);
  //clutter_actor_unrealize (g_viewport);

  clutter_container_add_actor (CLUTTER_CONTAINER (stage), bg);
  clutter_actor_show (bg);
  single_view_bg = bg;
  //g_list_free (children);
  view_pic (actor, FALSE);
}

#ifdef HAVE_GESTURE
static void
start_rotate_viewport (TidyViewport* viewport, gboolean right)
{
  tidy_viewport_get_origin (viewport, &start, NULL, NULL);
  gint old_target = target;
  target = right ? target + 10000 : target - 10000;
  slide_viewport (viewport, old_target, target, 10000 / RECT_GAP * g_speed);
}
#endif

static void
stop_viewport (TidyViewport* viewport, gboolean still)
{
  clutter_timeline_stop (timeline);
  if (still)
    return;

  tidy_viewport_get_origin (viewport, &start, NULL, NULL);
  gint x, old_target = target;
  tidy_viewport_get_origin (TIDY_VIEWPORT (viewport), &x, NULL, NULL);
  target = (x / RECT_GAP) * RECT_GAP + (target < start ? RECT_GAP : 0);
  if (target != x)
    slide_viewport (viewport, old_target, target, 0);
}

void
slide_viewport (TidyViewport* viewport, gint old_target, gint new_target, gint frames)
{
  tidy_viewport_get_origin (viewport, &start, NULL, NULL);
  if (clutter_timeline_is_playing (timeline))
    {
      clutter_timeline_stop (timeline);
      if (frames > 0)
        clutter_timeline_set_duration (timeline, frames * 1000 / FPS);
      else if (ABS (new_target - start) > ABS (old_target - start))
        clutter_timeline_set_duration (timeline,
                                       ((FRAMES -clutter_timeline_get_elapsed_time (timeline) * FPS / 1000) + FRAMES / 2) * 1000 / FPS);
      else
        clutter_timeline_set_duration (timeline,
                                       (FRAMES - MAX (1,clutter_timeline_get_elapsed_time (timeline) * FPS / 1000 - FRAMES / 2)) * 1000 / FPS);
      clutter_timeline_rewind (timeline);
      clutter_timeline_start (timeline);
    }
  else
    {
      if (frames > 0)
        clutter_timeline_set_duration (timeline, frames * 1000 / FPS);
      else
        clutter_timeline_set_duration (timeline, FRAMES * 1000 / FPS);
      clutter_timeline_rewind (timeline);
      clutter_timeline_start (timeline);
    }
}

static void
stage_button_press_event_cb (ClutterActor *actor, ClutterButtonEvent *event,
                             TidyViewport *viewport)
{
  int value;

  puts ("button pressed.");

  pressed = TRUE;
  pressed_x = event->x;
  pressed_y = event->y;
  pressed_device = clutter_event_get_device_id ((ClutterEvent*)event);
  if (!is_in_single_view_mode())
    {
      if (event->click_count == 1)
        {
          tidy_viewport_get_origin (TIDY_VIEWPORT (viewport), &value, NULL, NULL);
          pressed_viewport_x = value;
        }
      if (clutter_timeline_is_playing (timeline))
        stop_viewport (TIDY_VIEWPORT(viewport), TRUE);
    }
  else
    {
      clutter_actor_get_position (single_pic, &pressed_viewport_x, &pressed_viewport_y);
    }
}

static void
stage_button_release_event_cb (ClutterActor *actor, ClutterButtonEvent *event,
                               TidyViewport *viewport)
{
  if (!pressed)
    return;

  pressed = FALSE;
  pinch_center_set = FALSE;
  rotation_center_set = FALSE;

  puts ("button released.");
  if (is_in_single_view_mode())
    {
      if (event->click_count == 2)
        if (zoom_at_point (0, 0, -1))
          exit_single_view();
      return;
    }

  if (event->click_count == 2)
    {
      switch_to_single_view (CLUTTER_GROUP(pic_group));
      return;
    }

  gint x, old_target = target;
  tidy_viewport_get_origin (TIDY_VIEWPORT (viewport), &x, NULL, NULL);
  target = (x / RECT_GAP) * RECT_GAP + (event->x < pressed_x ? RECT_GAP : 0);
  if (target != x)
    slide_viewport (viewport, old_target, target, 0);
  else
    {
      /* single click */
      if (event->x > stage_width * 3 / 5)
        {
          target += RECT_GAP;
        }
      else if (event->x < stage_width * 2 / 5)
        {
          target -= RECT_GAP;
        }
      if (target != old_target)
        slide_viewport (viewport, old_target, target, 0);
    }
}

static void
stage_motion_event_cb (ClutterActor *actor, ClutterMotionEvent *event,
                       TidyViewport *viewport)
{
#ifndef HAVE_DISABLE_MOTION
  gint y, z;

  if (!pressed)
    return;
  /* MPX support, check if we're moving the same pointer */
  if (pressed_device != clutter_event_get_device_id ((ClutterEvent*)event))
    return;

  if (!is_in_single_view_mode())
    {
#ifndef HAVE_TWO_FINGER_SLIDE
      tidy_viewport_get_origin (TIDY_VIEWPORT (viewport), NULL, &y, &z);
      tidy_viewport_set_origin (TIDY_VIEWPORT (viewport),
                                pressed_viewport_x - event->x + pressed_x, y, z);
#endif
    }
  else
    {
#ifndef DISABLE_MOTION_SV
      DBG ("motion");

      clutter_actor_set_position (single_pic, pressed_viewport_x + event->x - pressed_x,
                                  pressed_viewport_y + event->y - pressed_y);

#endif
    }
#endif
}

static void
stage_key_press_event_cb (ClutterActor *actor, ClutterKeyEvent *event,
                          TidyViewport *viewport)
{
  if (!is_in_single_view_mode())
    {
      gint old_target;

      old_target = target;

      switch (event->keyval)
        {
        case CLUTTER_Left :
        case CLUTTER_KP_Left :
          target -= RECT_GAP;
          break;
        case CLUTTER_Right :
        case CLUTTER_KP_Right :
          target += RECT_GAP;
          break;
        case CLUTTER_KP_Enter:
        case CLUTTER_Return:
          switch_to_single_view (CLUTTER_GROUP(pic_group));
          break;
        case CLUTTER_Escape:
          clutter_main_quit ();
          break;
        }

      if (target != old_target)
        slide_viewport (viewport, old_target, target, 0);
    }
  else /* single view mode */
    {
      switch (event->keyval)
        {
        case CLUTTER_Escape:
          exit_single_view ();
          break;
        case CLUTTER_Left :
        case CLUTTER_KP_Left :
          single_view_navigate (FALSE);
          break;
        case CLUTTER_Right :
        case CLUTTER_KP_Right :
          single_view_navigate (TRUE);
          break;
        }
    }
}

static void
new_frame_cb (ClutterTimeline *timeline, gint frame_num, ClutterActor *viewport)
{
  gint y, z;
  gdouble progress;

  tidy_viewport_get_origin (TIDY_VIEWPORT (viewport), NULL, &y, &z);
  progress = clutter_timeline_get_progress (timeline);
  tidy_viewport_set_origin (TIDY_VIEWPORT (viewport),
                            (gint)((progress * (gdouble)target) +
                            ((1.0 - progress) * (gdouble)start)), y, z);
}

static void
viewport_x_origin_notify_cb (TidyViewport *viewport,
                             GParamSpec *args1,
                             ClutterActor *group)
{
  GList *children, *c;
  gfloat width;
  int origin_x;

  tidy_viewport_get_origin (viewport, &origin_x, NULL, NULL);
  clutter_actor_get_clip (CLUTTER_ACTOR (viewport),
                          NULL, NULL, &width, NULL);

  children = clutter_container_get_children (CLUTTER_CONTAINER (group));
  int half = total_pics / 2 * RECT_GAP;
  gint base = (width-RECT_W) / 2 / RECT_GAP * 2 * RECT_GAP + 2 * RECT_GAP;
  for (c = children; c; c = c->next)
    {
      gint x, height;
      gdouble pos;
      ClutterActor *actor;

      actor = (ClutterActor *)c->data;

      /* Get actor position with respect to viewport origin */
      x = clutter_actor_get_x (actor) - origin_x;

      /* loop stuff */
      if (x - base / 2 > half)
        {
          clutter_actor_set_x (actor, origin_x + (x - half) - half - RECT_GAP);
          x -= 2 * half + RECT_GAP;
        }
      else if (x - base / 2 < -half)
        {
          clutter_actor_set_x (actor, origin_x + (x + half) + half + RECT_GAP);
          x += 2 * half + RECT_GAP;
        }

      height = clutter_actor_get_height (actor);
      pos = (((gdouble)x / base) - 0.5) * 2.0;

      /* Apply a function that transforms the actor depending on its
       * viewport position.
       */
      pos = CLAMP(pos * 3.0, -0.5, 0.5);
      clutter_actor_set_rotation (actor, CLUTTER_Y_AXIS, -pos * 120.0,
                                  RECT_W/2, 0, RECT_H/2);

#ifdef HAVE_GAMMA
      if (ABS(pos) < 0.1)
        {
          g_object_set (G_OBJECT (actor), "gamma", 255, NULL);
        }
      else
        {
          g_object_set (G_OBJECT (actor), "gamma", 0x70, NULL);
        }
#endif
      clutter_actor_set_depth (actor, -ABS(pos) * RECT_W);

    }
  g_list_free (children);
}


static void
add_pics (ClutterActor *stage, ClutterActor *group, const char* img_folder)
{
  GFileEnumerator *file_enumerator;
  GFile* root = g_file_new_for_path (img_folder);
  const char *mime_type, *name;
  ClutterActor* actor;
  int i = 0;
  total_pics = 0;
  file_enumerator = g_file_enumerate_children (root,
                                               G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE ","
                                               G_FILE_ATTRIBUTE_STANDARD_NAME,
                                               0, NULL, NULL);
  GFileInfo* file_info = g_file_enumerator_next_file (file_enumerator, NULL, NULL);

  while (file_info != NULL)
    {
      mime_type = g_file_info_get_content_type (file_info);
      name = g_file_info_get_name (file_info);

      if (strstr(mime_type, "image/") == mime_type)
        {
          GFile* file = g_file_get_child (root, name);
          const char* path = g_file_get_path (file);
          DBG ("adding %s (%s) ... ", name, mime_type);

          if ((actor = tidy_dark_texture_new_from_file(path, NULL)))
            {
              gint width, height;
              clutter_texture_get_base_size (CLUTTER_TEXTURE(actor), &width, &height);
              gfloat disp_height = height * RECT_W / width;
              clutter_container_add_actor (CLUTTER_CONTAINER (group), actor);

              clutter_actor_set_size (actor, RECT_W, disp_height);
              clutter_actor_set_position (actor, i * RECT_GAP,
                                          CLUTTER_STAGE_HEIGHT ()/2 - disp_height/2);

              pic_actors = g_list_append (pic_actors, actor);
              i++;
              DBG  ("\n");
            }
          else
            {
              DBG  ("failed\n");
            }
          g_free ((gpointer)path);
          g_object_unref (file);
       }
      g_object_unref (file_info);
      file_info = g_file_enumerator_next_file (file_enumerator, NULL, NULL);
    }
  g_file_enumerator_close (file_enumerator, NULL, NULL);
  g_object_unref (file_enumerator);
  g_object_unref (root);
  total_pics = i;
}

#if 0
static void
add_rects (ClutterActor *stage, ClutterActor *group)
{
  gint i;

  ClutterColor colour = { 0x72, 0x9f, 0xcf, 0xff };

  for (i = 0; i < RECT_N; i++)
    {
      ClutterActor *rect = clutter_rectangle_new_with_color (&colour);

      clutter_container_add_actor (CLUTTER_CONTAINER (group), rect);

      clutter_actor_set_size (rect, RECT_W, RECT_H);
      clutter_actor_set_position (rect, i * RECT_GAP,
                                  CLUTTER_STAGE_HEIGHT ()/2 - RECT_H/2);

      colour.red = g_random_int_range (0, 255);
      colour.green = g_random_int_range (0, 255);
    }
}
#endif

void
usage(const char* self_name)
{
  printf("%s: [-h] [-r <double click radius>] [-e <event sample frequency(120)>] \
[-s <speed (default 30, bigger is slower)>] \
[image folder]\n", self_name);
}


int
main (int argc, char **argv)
{
  ClutterActor *stage, *viewport, *group;
  ClutterColor stage_color = { 0x34, 0x39, 0x39, 0xff };
  gboolean hide_cursor = FALSE, fullscreen = TRUE;
  int double_click_radius = 10, c, sample_freq = 120;

  while ((c = getopt (argc, argv, "btpofhr:s:e:d:a:")) != -1)
    switch (c)
      {
      case 'd':
        rotate_test_fps = atoi (optarg);
        break;
      case 'a':
        rotate_test_angle_delta = atoi (optarg);
        break;
      case 'b':
        test_mode = TRUE;
        break;
      case 't':
        do_rotation_timeline = TRUE;
        break;
      case 'p':
        pinch_only = TRUE;
        break;
      case 'o':
        rotate_only = TRUE;
        break;
      case 'f':
        fullscreen = FALSE;
        break;
      case 'h':
        hide_cursor = TRUE;
        break;
      case 'r':
        double_click_radius = atoi(optarg);
        break;
      case 'e':
        sample_freq = atoi(optarg);
        break;
      case 's':
        g_speed = atoi(optarg);
        break;
      case '?':
        if (optopt == 'r')
          fprintf (stderr, "Option -%c requires an argument.\n", optopt);
        else if (isprint (optopt))
          fprintf (stderr, "Unknown option `-%c'.\n", optopt);
        else
          fprintf (stderr,
                   "Unknown option character `\\x%x'.\n",
                   optopt);
        usage (argv[0]);
        return 1;
      default:
        usage (argv[0]);
        abort ();
      }

  /* these four steps are to be called in this specific order
   * to get XI2 working
   */
  g_type_init();

  clutter_x11_enable_xinput();

  clutter_init (&argc, &argv);

  stage = clutter_stage_get_default ();

  clutter_stage_set_throttle_motion_events(CLUTTER_STAGE(stage), FALSE);

  clutter_set_default_frame_rate(60);

  if (hide_cursor)
    clutter_stage_hide_cursor (CLUTTER_STAGE(stage));

  DBG ("SETTING DOUBLE CLICK DISTANCE = %d\n", double_click_radius);
  clutter_backend_set_double_click_distance (clutter_get_default_backend (),
                                             double_click_radius);

  if (optind < argc)
    folder = argv[optind];
  else
    folder = EOM_SAMPLE_DIR;
  printf ("LOADING FROM %s\n", folder);

  clutter_stage_set_color (CLUTTER_STAGE (stage), &stage_color);
  //  clutter_stage_set_use_fog (CLUTTER_STAGE (stage), TRUE);
  clutter_stage_set_fullscreen (CLUTTER_STAGE(stage), fullscreen);

  /* we need motion events on the stage only,
   * to save the pick/paint.
   */
  clutter_set_motion_events_enabled (FALSE);

  if (!fullscreen) {
    clutter_actor_set_size (stage, 800, 600);
    construct_stage ();
  } else {
    g_signal_connect (stage, "fullscreen", G_CALLBACK(construct_stage), NULL);
    clutter_actor_show_all (stage);
  }

  clutter_main ();

  return 0;
}

void
construct_stage()
{
  static gboolean constructed = FALSE;
  if (constructed)
    return;
  constructed = TRUE;
  ClutterActor *stage, *viewport, *group;
  stage = clutter_stage_get_default ();

  stage_width = clutter_actor_get_width (stage);
  stage_height = clutter_actor_get_height (stage);

  viewport = tidy_viewport_new ();
  g_viewport = viewport;

  clutter_actor_set_clip (viewport, 0, 0, stage_width, stage_height);

  pic_group = group = tidy_depth_group_new ();

  add_pics (stage, group, folder);
  clutter_container_add_actor (CLUTTER_CONTAINER (viewport), group);
  g_signal_connect (viewport, "notify::x-origin",
                    G_CALLBACK (viewport_x_origin_notify_cb), group);

  clutter_container_add_actor (CLUTTER_CONTAINER (stage), viewport);

  clutter_actor_set_reactive (group, TRUE);

  clutter_actor_show_all (group);
  clutter_actor_show_all (viewport);
  clutter_actor_show_all (stage);

  tidy_viewport_set_origin (TIDY_VIEWPORT (viewport), RECT_W / 2, 0, 0);
  target = RECT_W / 2;
  timeline = clutter_timeline_new (FRAMES * 1000 / FPS);
  g_signal_connect (timeline, "new_frame",
                    G_CALLBACK (new_frame_cb), viewport);

  g_signal_connect (stage, "key-press-event",
                    G_CALLBACK (stage_key_press_event_cb), viewport);
  g_signal_connect (stage, "button-press-event",
                    G_CALLBACK (stage_button_press_event_cb), viewport);
  g_signal_connect (stage, "button-release-event",
                    G_CALLBACK (stage_button_release_event_cb), viewport);
  g_signal_connect (stage, "motion-event",
                    G_CALLBACK (stage_motion_event_cb), viewport);

  g_object_notify (G_OBJECT (viewport), "x-origin");

#ifdef HAVE_GESTURE
  ClutterGesture *gesture;

  gesture = clutter_gesture_new(CLUTTER_ACTOR(stage));
  clutter_gesture_set_gesture_mask(gesture, stage,
                                   GESTURE_MASK_SLIDE |
                                   GESTURE_MASK_PINCH | GESTURE_MASK_ROTATE);

  g_signal_connect (gesture, "gesture-slide-event",
                    G_CALLBACK (gesture_slide_cb), (gpointer)0x11223344);

  if (!rotate_only)
    g_signal_connect (gesture, "gesture-pinch-event",
                      G_CALLBACK (gesture_pinch_cb), (gpointer)0x11223344);
  if (!pinch_only)
    g_signal_connect (gesture, "gesture-rotate-event",
                      G_CALLBACK (gesture_rotate_cb), (gpointer)0x11223344);

#ifdef HAVE_TWO_FINGER_SLIDE
  g_signal_connect (gesture, "gesture-navigate-event",
                    G_CALLBACK (gesture_navigate_cb), (gpointer)viewport);

#endif

#endif
}
