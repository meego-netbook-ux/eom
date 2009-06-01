/* pictable: clutter physics demo application
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
 * clutter-gesture support by:
 *   Roger WANG <roger.wang@intel.com>
 *
 * from clutter-box2d example by:
 *   Emmanuele Bassi <ebassi@linux.intel.com>
 *   Øyvind Kolås <pippin@linux.intel.com>
 */

#include <stdlib.h>
#include <stdarg.h>
#include <clutter/clutter.h>
#include "clutter-box2d.h"
#include "blockbox.h"

#define BOX2D_MANIPULATION

typedef enum
{
  None,
  Direct
#ifdef BOX2D_MANIPULATION
  , Box2D
#endif
} ManipulationMode;

static ManipulationMode  mode              = None;
static ClutterActor     *manipulated_actor = NULL;
static ClutterUnit       orig_x, orig_y;
static ClutterUnit       start_x, start_y;
static ClutterUnit       orig_rotation;

#ifdef BOX2D_MANIPULATION
ClutterBox2DJoint *mouse_joint = NULL;
#endif

ClutterActor *
actor_manipulator_get_victim (void)
{
  return manipulated_actor;
}

static gboolean
should_be_manipulated (ClutterActor *actor)
{
  ClutterActor *ancestor;

  if (!actor ||
      !clutter_actor_get_parent (actor))
    return FALSE;

  for (ancestor = actor; ancestor;
       ancestor = clutter_actor_get_parent (ancestor))
    {
      if (g_object_get_data (G_OBJECT (ancestor), "_") != NULL)
        return FALSE;
    }

  return TRUE;
}


static void
set_opacity (ClutterActor *actor,
             gdouble       value)
{
  clutter_actor_set_opacity (actor, value);
}

static void
set_rotation (ClutterActor *actor,
              gdouble       value)
{
  clutter_actor_set_rotation (actor, CLUTTER_Z_AXIS, value, 0, 0, 0);
}

static
gboolean
action_remove (ClutterActor *action,
               ClutterEvent *event,
               gpointer      userdata)
{
  clutter_actor_destroy (userdata);
  return FALSE;
}

static
gboolean
action_set_linear_velocity (ClutterActor *action,
                            ClutterEvent *event,
                            gpointer      userdata)
{
  ClutterActor *actor;
  ClutterBox2D *box2d;
  ClutterVertex velocity = { CLUTTER_UNITS_FROM_FLOAT (150.0f),
                             CLUTTER_UNITS_FROM_FLOAT (-150.0f) };
  actor = CLUTTER_ACTOR (userdata);
  box2d = CLUTTER_BOX2D (clutter_actor_get_parent (actor));

  clutter_container_child_set (CLUTTER_CONTAINER (box2d),
                               actor, "linear-velocity", &velocity, NULL);

  return TRUE;
}


static
gboolean
action_set_dynamic (ClutterActor *action,
                    ClutterEvent *event,
                    gpointer      userdata)
{
  ClutterActor *actor;
  ClutterBox2D *box2d;

  actor = CLUTTER_ACTOR (userdata);
  box2d = CLUTTER_BOX2D (clutter_actor_get_parent (actor));

  clutter_container_child_set (CLUTTER_CONTAINER (box2d),
                               actor, "mode", CLUTTER_BOX2D_DYNAMIC, NULL);
  clutter_container_child_set (CLUTTER_CONTAINER (box2d),
                               actor, "manipulatable", TRUE, NULL);

  return FALSE;
}

static
gboolean
action_set_static (ClutterActor *action,
                   ClutterEvent *event,
                   gpointer      userdata)
{
  ClutterActor *actor;
  ClutterBox2D *box2d;

  actor = CLUTTER_ACTOR (userdata);
  box2d = CLUTTER_BOX2D (clutter_actor_get_parent (actor));

  clutter_container_child_set (CLUTTER_CONTAINER (box2d), actor,
                               "mode", CLUTTER_BOX2D_STATIC, NULL);

  return FALSE;
}


static
gboolean
action_add_rectangle (ClutterActor *action,
                      ClutterEvent *event,
                      gpointer      userdata)
{
  ClutterActor *group = CLUTTER_ACTOR (userdata);
  ClutterActor *box;

  box = clutter_rectangle_new ();
  clutter_actor_set_size (box, 100, 100);
  clutter_actor_set_position (box, event->button.x, event->button.y);
  clutter_group_add (CLUTTER_GROUP (group), box);

  return FALSE;
}

static
gboolean
action_add_text (ClutterActor *action,
                 ClutterEvent *event,
                 gpointer      userdata)
{
  ClutterActor *group = CLUTTER_ACTOR (userdata);
  ClutterActor *title;
  ClutterColor  color;

  clutter_color_from_string (&color, "#888");

  title = clutter_text_new_full ("Sans 30px", "fnord", &color);

  clutter_actor_set_position (title, event->button.x, event->button.y);
  clutter_group_add (CLUTTER_GROUP (group), title);
  return FALSE;
}

static
gboolean
action_add_image (ClutterActor *action,
                  ClutterEvent *event,
                  gpointer      userdata)
{
  ClutterActor *group = CLUTTER_ACTOR (userdata);
  ClutterActor *actor;

  actor = clutter_texture_new_from_file (ASSETS_DIR "redhand.png", NULL);
  clutter_actor_set_position (actor, event->button.x, event->button.y);
  clutter_group_add (CLUTTER_GROUP (group), actor);
  return FALSE;
}


#if 0
static
gboolean
action_add_block_tree (ClutterActor *action,
                       ClutterEvent *event,
                       gpointer      userdata)
{
  ClutterActor *actor;

  actor = block_tree_new ("foo");
  clutter_group_add (CLUTTER_GROUP (clutter_stage_get_default ()), actor);
  clutter_actor_set_position (actor, 20, 40);
  return FALSE;
}
#endif


static
gboolean
action_zero_gravity (ClutterActor *action,
                     ClutterEvent *event,
                     gpointer      userdata)
{
  ClutterBox2D *box2d  = CLUTTER_BOX2D (scene_get_group ());
  ClutterVertex vertex = { 0, 0, 0 };

  g_object_set (G_OBJECT (box2d), "gravity", &vertex, NULL);

  return FALSE;
}

static gboolean
actor_manipulator_press (ClutterActor *stage,
                         ClutterEvent *event,
                         gpointer      data)
{
  ClutterActor *actor;

  actor = clutter_stage_get_actor_at_pos (CLUTTER_STAGE (stage),
                                          event->button.x,
                                          event->button.y);

  const gchar* name = clutter_actor_get_name(actor);
  if (name && (!strcmp(name, "margin") || !strcmp(name, "pic")))
    /* we should pick the outer frame here */
    actor = clutter_actor_get_parent (actor);
  else if (actor == stage ||
      CLUTTER_IS_GROUP (actor) && name && strcmp(name, "frame"))
    {
      return TRUE;
    }

  if (actor == NULL)
    {
      return FALSE;
    }

  if (!should_be_manipulated (actor))
    return FALSE;

  manipulated_actor = actor;

  clutter_actor_get_positionu (actor, &orig_x, &orig_y);
  orig_rotation = clutter_actor_get_rotationu (actor, CLUTTER_Z_AXIS, NULL,
                                               NULL,
                                               NULL);

  start_x = CLUTTER_UNITS_FROM_INT (event->button.x);
  start_y = CLUTTER_UNITS_FROM_INT (event->button.y);

  clutter_actor_transform_stage_point (
    clutter_actor_get_parent (manipulated_actor),
    start_x, start_y,
    &start_x, &start_y);


  mode = Direct;


#ifdef BOX2D_MANIPULATION
  /* Use Box2D manipulation if the actor is dynamic, and the physics
   * engine is running
   */
  if (CLUTTER_IS_BOX2D (scene_get_group ()) &&
      clutter_box2d_get_simulating (CLUTTER_BOX2D (scene_get_group ())))
    {
      ClutterBox2D *box2d  = CLUTTER_BOX2D (scene_get_group ());
      /*ClutterVertex target = { start_x, start_y };*/
      gint type;

      clutter_container_child_get (CLUTTER_CONTAINER (box2d),
                                   manipulated_actor, "mode", &type, NULL);

      if (type == CLUTTER_BOX2D_DYNAMIC)
        {
#if 0
            mouse_joint = clutter_box2d_add_mouse_joint (CLUTTER_BOX2D (
                                                           scene_get_group ()),
                                                         manipulated_actor,
                                                         &target);
#endif
            mode = None; /*Box2D;*/
            manipulated_actor = NULL;
            return FALSE;
        }
    }
#endif
  clutter_grab_pointer (stage);

  return TRUE;
}

static gboolean
actor_manipulator_motion (ClutterActor *stage,
                          ClutterEvent *event,
                          gpointer      data)
{
  if (manipulated_actor)
    {
      ClutterUnit x;
      ClutterUnit y;
      ClutterUnit dx;
      ClutterUnit dy;

      x = CLUTTER_UNITS_FROM_INT (event->button.x);
      y = CLUTTER_UNITS_FROM_INT (event->button.y);

      clutter_actor_transform_stage_point (
        clutter_actor_get_parent (manipulated_actor),
        x, y,
        &x, &y);

      dx = x - start_x;
      dy = y - start_y;

      switch (mode)
        {
#ifdef BOX2D_MANIPULATION
          case Box2D:
          {
            ClutterVertex target = { x, y };
            clutter_box2d_mouse_joint_update_target (mouse_joint, &target);
            break;
          }
#endif

          case Direct:
            if (clutter_event_get_state (event) & CLUTTER_BUTTON1_MASK)
              {
                x = orig_x + dx;
                y = orig_y + dy;

                clutter_actor_set_positionu (manipulated_actor, x, y);
              }
            else if (clutter_event_get_state (event) & CLUTTER_BUTTON2_MASK)
              {
                clutter_actor_set_rotationu (manipulated_actor, CLUTTER_Z_AXIS,
                                             orig_rotation + dx, 0, 0, 0);
              }
            break;

          case None:
            g_print ("we shouldn't be doing %s in None mode\n", G_STRLOC);
            return FALSE;
        }
    }
  return FALSE;
}

static gboolean
actor_manipulator_release (ClutterActor *stage,
                           ClutterEvent *event,
                           gpointer      data)
{
  if (manipulated_actor)
    {
      clutter_ungrab_pointer ();
      manipulated_actor = NULL;

      switch (mode)
        {
#ifdef BOX2D_MANIPULATION
          case Box2D:
          {
            if (mouse_joint)
              clutter_box2d_joint_destroy (mouse_joint);
            mouse_joint = NULL;
            break;
          }
#endif

          case Direct:
          case None:
            mode  = None;
            return FALSE;
            break;
        }
      mode              = None;
    }
  return TRUE;
}

void
actor_manipulator_init (ClutterActor *stage)
{
  g_signal_connect (stage, "button-press-event",
                    G_CALLBACK (actor_manipulator_press), NULL);
  g_signal_connect (stage, "motion-event",
                    G_CALLBACK (actor_manipulator_motion), NULL);
  g_signal_connect (stage, "button-release-event",
                    G_CALLBACK (actor_manipulator_release), NULL);
}
