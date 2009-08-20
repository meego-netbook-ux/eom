#include "config.h"

#include <string.h>
#include <gio/gio.h>

#include <clutter/clutter.h>
#include <clutter/x11/clutter-x11.h>

#ifdef HAVE_GESTURE
#include <clutter-gesture/clutter-gesture.h>
#endif

ClutterActor* single_pic;
gboolean pressed = FALSE;

static gfloat pressed_x, pressed_y, pressed_viewport_x, pressed_viewport_y, pressed_device;


static void
stage_button_press_event_cb (ClutterActor *actor, ClutterButtonEvent *event,
                             gpointer data)
{
  pressed = TRUE;
  pressed_x = event->x;
  pressed_y = event->y;

  clutter_actor_get_position (single_pic, &pressed_viewport_x, &pressed_viewport_y);

}


static void
stage_button_release_event_cb (ClutterActor *actor, ClutterButtonEvent *event,
                               gpointer data)
{
  pressed = FALSE;
}

static void
stage_motion_event_cb (ClutterActor *actor, ClutterMotionEvent *event,
                       gpointer data)
{
  if (!pressed)
    return;

  clutter_actor_set_position (single_pic, pressed_viewport_x + event->x - pressed_x,
                              pressed_viewport_y + event->y - pressed_y);
}

#ifdef HAVE_GESTURE

static void
first_half_completed (ClutterTimeline *timeline,
                      gpointer data)
{
  ClutterGeometry geo;
  gint dest_x = (gint)data;
  gint cur_x;

  clutter_actor_get_geometry (single_pic, &geo);
  cur_x = geo.x < 0 ? CLUTTER_STAGE_WIDTH () : -geo.x;

  clutter_actor_set_position (single_pic, cur_x, geo.y);

  clutter_actor_animate (single_pic, CLUTTER_LINEAR,
                         abs (cur_x - dest_x),
                         "x", (gfloat)dest_x,
                         NULL);
}

static gboolean
gesture_slide_cb (ClutterGesture    *gesture,
                  ClutterGestureEvent    *event,
                  gpointer         data)
{
  ClutterGeometry geo;
  gint dest_x;

  if (event && event->type == GESTURE_SLIDE)
    {
      ClutterGestureSlideEvent *slide = (ClutterGestureSlideEvent *)event;

      clutter_actor_get_geometry (single_pic, &geo);
      switch (slide->direction)
        {
        case SLIDE_LEFT:
          dest_x = - geo.width;
          break;
        case SLIDE_RIGHT:
          dest_x = CLUTTER_STAGE_WIDTH ();
          break;
        default:
          return FALSE;
        }
      ClutterAnimation* ani = clutter_actor_animate (single_pic, CLUTTER_LINEAR,
                                                     abs (geo.x - dest_x),
                                                     "x", (gfloat)dest_x,
                                                     NULL);
      ClutterTimeline* tml = clutter_animation_get_timeline (ani);
      g_signal_connect (tml, "completed", G_CALLBACK(first_half_completed),
                        geo.x);
      return TRUE;
    }
  return FALSE;
}

#endif

int
main (int argc, char **argv)
{
  ClutterColor stage_color = { 0x34, 0x39, 0x39, 0xff };
  ClutterColor test_block_color = { 0x7f, 0xae, 0xff, 0xff };
  ClutterActor* clone;

  clutter_init (&argc, &argv);

  ClutterActor* stage = clutter_stage_get_default ();
  clutter_stage_set_color (CLUTTER_STAGE (stage), &stage_color);
  clutter_stage_set_fullscreen (CLUTTER_STAGE(stage), TRUE);
  clutter_actor_show (stage);

  if (argc > 1 && (clone = clutter_texture_new_from_file(argv[1], NULL)) != NULL)
    {
    }
  else
    clone = clutter_rectangle_new_with_color (&test_block_color);

  single_pic = clone;

  int height = 700, width = 1000;

  clutter_actor_set_size (clone, width, height);
  clutter_container_add_actor (CLUTTER_CONTAINER (stage), clone);

  clutter_actor_show (clone);

  g_signal_connect (stage, "button-press-event",
                    G_CALLBACK (stage_button_press_event_cb), NULL);
  g_signal_connect (stage, "button-release-event",
                    G_CALLBACK (stage_button_release_event_cb), NULL);
  g_signal_connect (stage, "motion-event",
                    G_CALLBACK (stage_motion_event_cb), NULL);

#ifdef HAVE_GESTURE
  ClutterGesture *gesture;

  gesture = clutter_gesture_new(CLUTTER_ACTOR(stage));
  clutter_gesture_set_gesture_mask(gesture, stage,
                                   GESTURE_MASK_SLIDE);
  g_signal_connect (gesture, "gesture-slide-event",
                    G_CALLBACK (gesture_slide_cb), (gpointer)0x11223344);

  clutter_main ();
#endif

  return 0;
}
