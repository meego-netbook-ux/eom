/* stuff - a content retrieval and management experiment.
 *
 * Copyright Øyvind Kolås <pippin@gimp.org> 2007-2008
 * Licensed under the GPL v3 or greater.
 */

#include <glib.h>
#include <clutter/clutter.h>
#include <clutter-box2d/clutter-box2d.h>
#include "blockbox.h"

typedef struct
{
  void (*func
        )(void*);
  gpointer data;
} DeferredCall;

static gboolean
deferred_call_invoker (gpointer data)
{
  DeferredCall *call_data;

  call_data = data;
  call_data->func (call_data->data);
  return FALSE;
}

void
stuff_deferred_call (void     (*func)(void*) ,
                     gpointer data)
{
  DeferredCall *call_data;

  call_data = g_malloc (sizeof (DeferredCall));

  call_data->func = func;
  call_data->data = data;

  g_idle_add_full (G_PRIORITY_HIGH,
                   deferred_call_invoker,
                   call_data,
                   g_free);
}

/****************************************************************/

ClutterActor *
add_hand (ClutterActor *group,
          gint          x,
          gint          y)
{
  ClutterActor     *actor;

  actor = clutter_texture_new_from_file (ASSETS_DIR "redhand.png", NULL);
  clutter_group_add (CLUTTER_GROUP (group), actor);

  clutter_actor_set_opacity (actor, 1.0 * 255);
  clutter_actor_set_position (actor, x, y);

  clutter_container_child_set (CLUTTER_CONTAINER (group), actor,
                               "manipulatable", TRUE,
                               "mode", CLUTTER_BOX2D_DYNAMIC, NULL);
  return actor;
}

static void
add_static_box (ClutterActor *group,
                gint          x,
                gint          y,
                gint          width,
                gint          height)
{
  ClutterActor *box;
  box = clutter_rectangle_new ();
  clutter_actor_set_size (box, width, height);
  clutter_actor_set_position (box, x, y);
  clutter_group_add (CLUTTER_GROUP (group), box);


  clutter_container_child_set (CLUTTER_CONTAINER (group), box,
                               "mode", CLUTTER_BOX2D_STATIC, NULL);

}

void
add_cage (ClutterActor *group,
          gboolean      roof)
{
  ClutterActor *stage;
  gint width, height;

  stage = clutter_stage_get_default ();
  width = clutter_actor_get_width (stage);
  height = clutter_actor_get_height (stage);

  if (roof)
    {
      add_static_box (group, -100, -100, width + 200, 100);
    }
  else
    {
      add_static_box (group, -100, -height*(3-1)-100, width + 200, 100);
    }
  add_static_box (group, -100, height, width + 200, 100);

  add_static_box (group, -100, -(height*(5-1)) , 100, height * 5);
  add_static_box (group, width, -(height*(5-1)) , 100, height * 5);
}
