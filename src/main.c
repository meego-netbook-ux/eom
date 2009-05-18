#include "config.h"
#include <string.h>
#include <gio/gio.h>
#include <clutter/clutter.h>
#include <tidy/tidy-viewport.h>
#include <tidy/tidy-depth-group.h>

#ifdef HAVE_GESTURE
#include <clutter-gesture/clutter-gesture.h>
#endif

#define RECT_W 400
#define RECT_H 400
#define RECT_N 20
#define RECT_GAP 50
#define FRAMES 20

static ClutterTimeline *timeline;
static gint start, target;
static gboolean pressed = FALSE;
static gint pressed_x, pressed_y, pressed_viewport_x, pressed_viewport_y;
static ClutterActor* pic_group, *single_pic, *current_actor, *single_view_bg, *g_viewport;
static gint single_view_x0, single_view_y0;
static GList* pic_actors;
static gint total_pics;

void switch_to_single_view (ClutterGroup* group);
void single_view_navigate (gboolean next);
gboolean is_in_single_view_mode();
gboolean zoom_at_point (int x, int y, float scale);
void slide_viewport (TidyViewport* viewport, gint old_target, gint new_target, gint frames);

#ifdef HAVE_GESTURE
static void start_rotate_viewport (TidyViewport* viewport, gboolean right);

static gboolean
gesture_cb (ClutterGesture    *gesture,
            ClutterGestureEvent    *event,
            gpointer         data)
{
  /* const char *gesture_name[] = {"dummy", */
  /*                               "SLIDE", "PINCH", "WINDOW", "ANY"}; */

  const char *slide_dir_name [] = {
    "dummy",
    "SLIDE_UP", "SLIDE_DOWN", "SLIDE_LEFT", "SLIDE_RIGHT"};


  printf("gesture_cb: event pointer %p\n", event);
  if (event && event->type == GESTURE_SLIDE)
    {
      ClutterGestureSlideEvent *slide = (ClutterGestureSlideEvent *)event;
      printf("slide direction :%s\n", slide_dir_name[slide->direction]);
      switch (slide->direction)
        {
        case SLIDE_DOWN:
          if (!is_in_single_view_mode())
            switch_to_single_view (CLUTTER_GROUP(pic_group));
          else
            zoom_at_point (slide->x_start, slide->y_start, 0.8);
          break;
        case SLIDE_LEFT:
        case SLIDE_RIGHT:
          if (is_in_single_view_mode())
            single_view_navigate (slide->direction == SLIDE_RIGHT);
          else
            start_rotate_viewport (TIDY_VIEWPORT(g_viewport), slide->direction == SLIDE_LEFT);
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
  gint x, y;

  clutter_actor_destroy (single_pic);
  single_pic = (ClutterActor*)clone;
  clutter_actor_get_position (clone, &x, &y);
  single_view_x0 = x;
  single_view_y0 = y;
}

static void
view_pic (ClutterActor* actor, gboolean from_right)
{
  guint width, height;
  gint new_view_x0, new_view_y0;
  ClutterActor* stage = clutter_stage_get_default();

  clutter_actor_get_size (actor, &width, &height);
  ClutterActor* clone = clutter_clone_new (actor);

  gint disp_height = height * CLUTTER_STAGE_WIDTH() / width;
  gint disp_width = width * CLUTTER_STAGE_HEIGHT() / height;

  if (disp_height > CLUTTER_STAGE_HEIGHT())
    {
      disp_height = CLUTTER_STAGE_HEIGHT();
      new_view_x0 = CLUTTER_STAGE_WIDTH ()/2 - disp_width/2;
      new_view_y0 = 0;
    }
  else
    {
      disp_width = CLUTTER_STAGE_WIDTH();
      new_view_x0 = 0;
      new_view_y0 = CLUTTER_STAGE_HEIGHT ()/2 - disp_height/2;
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
          clutter_actor_set_position (clone, CLUTTER_STAGE_WIDTH(), new_view_y0);
        }
      else
        {
          ani_1 = clutter_actor_animate(single_pic, CLUTTER_LINEAR, 250,
                                        "x", CLUTTER_STAGE_WIDTH(), NULL);
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

/* returns TRUE when we're already in original scale */
gboolean
zoom_at_point (int x, int y, float scale)
{
  ClutterGeometry geo;
  gdouble scale_x, scale_y;

  clutter_actor_get_geometry (single_pic, &geo);
  clutter_actor_get_scale (single_pic, &scale_x, &scale_y);

  if (scale < 0)
    {
      /* return to origin scale */
      if (geo.x == single_view_x0 && geo.y == single_view_y0
          && scale_x == 1 && scale_y == 1)
        return TRUE;

      clutter_actor_set_position (single_pic, single_view_x0, single_view_y0);
      clutter_actor_set_scale (single_pic, 1, 1);
    }
  else
    {
      int newx = geo.x - ( x - CLUTTER_STAGE_WIDTH() / 2 );
      int newy = geo.y - ( y - CLUTTER_STAGE_HEIGHT() / 2 );
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
  clutter_actor_destroy (single_pic);
  clutter_actor_destroy (single_view_bg);
  single_pic = NULL;
}

void
switch_to_single_view (ClutterGroup* group)
{
  ClutterActor *actor;
  guint width, height;
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
  actor = clutter_stage_get_actor_at_pos(CLUTTER_STAGE(stage), width / 2, height / 2);

  ClutterColor bg_color = { 0x34, 0x39, 0x39, 0xff };
  ClutterActor* bg = clutter_rectangle_new_with_color (&bg_color);
  clutter_actor_set_size (bg, CLUTTER_STAGE_WIDTH(), CLUTTER_STAGE_HEIGHT());
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
  slide_viewport (viewport, old_target, target, 10000 / RECT_GAP * 10);
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
        clutter_timeline_set_n_frames (timeline, frames);
      else if (ABS (new_target - start) > ABS (old_target - start))
        clutter_timeline_set_n_frames (timeline,
                                       (FRAMES -clutter_timeline_get_current_frame (timeline)) + FRAMES / 2);
      else
        clutter_timeline_set_n_frames (timeline,
                                       FRAMES - MAX (1,clutter_timeline_get_current_frame (timeline) - FRAMES / 2));
      clutter_timeline_rewind (timeline);
      clutter_timeline_start (timeline);
    }
  else
    {
      if (frames > 0)
        clutter_timeline_set_n_frames (timeline, frames);
      else
        clutter_timeline_set_n_frames (timeline, FRAMES);
      clutter_timeline_rewind (timeline);
      clutter_timeline_start (timeline);
    }
}

static void
stage_button_press_event_cb (ClutterActor *actor, ClutterButtonEvent *event,
                             TidyViewport *viewport)
{
  pressed = TRUE;
  pressed_x = event->x;
  pressed_y = event->y;
  if (!is_in_single_view_mode())
    {
      if (event->click_count == 1)
        tidy_viewport_get_origin (TIDY_VIEWPORT (viewport), &pressed_viewport_x, NULL, NULL);
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
  pressed = FALSE;
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
      if (event->x > CLUTTER_STAGE_WIDTH() * 3 / 5)
        {
          target += RECT_GAP;
        }
      else if (event->x < CLUTTER_STAGE_WIDTH() * 2 / 5)
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
  gint y, z;

  if (!pressed)
    return;
  if (!is_in_single_view_mode())
    {
      tidy_viewport_get_origin (TIDY_VIEWPORT (viewport), NULL, &y, &z);
      tidy_viewport_set_origin (TIDY_VIEWPORT (viewport),
                                pressed_viewport_x - event->x + pressed_x, y, z);
    }
  else
    {
      clutter_actor_set_position (single_pic, pressed_viewport_x + event->x - pressed_x,
                                  pressed_viewport_y + event->y - pressed_y);
    }
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
  gint origin_x, width;

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
      if (clutter_actor_get_name(actor))
        {
          /* this is the dark actor mask */
          if (ABS(pos) < 0.1)
            {
              clutter_actor_set_depth (actor, -ABS(pos) * RECT_W - 1);
              clutter_actor_set_opacity (actor, 0);
            }
          else
            {
              clutter_actor_set_depth (actor, -ABS(pos) * RECT_W + 1);
              clutter_actor_set_opacity (actor, 180);
            }
        }
      else
        {
          clutter_actor_set_depth (actor, -ABS(pos) * RECT_W);
        }
#endif
    }
  g_list_free (children);
}

#ifdef HAVE_GAMMA

static void
new_black_actor (ClutterActor* orig, ClutterActor* group)
{
  ClutterGeometry geo;

  ClutterColor colour = { 0x00, 0x00, 0x00, 0xFF };
  ClutterActor* actor = clutter_rectangle_new_with_color(&colour);
  clutter_actor_get_geometry (orig, &geo);
  clutter_actor_set_geometry (actor, &geo);
  clutter_container_add_actor (CLUTTER_CONTAINER (group), actor);
  clutter_actor_raise (actor, orig);
  clutter_actor_set_name (actor, "b");
}
#endif

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
          printf("adding %s (%s) ... ", name, mime_type);
          if ((actor = clutter_texture_new_from_file(path, NULL)))
            {
              gint width, height;
              clutter_texture_get_base_size (CLUTTER_TEXTURE(actor), &width, &height);
              gint disp_height = height * RECT_W / width;
              clutter_container_add_actor (CLUTTER_CONTAINER (group), actor);

              clutter_actor_set_size (actor, RECT_W, disp_height);
              clutter_actor_set_position (actor, i * RECT_GAP,
                                          CLUTTER_STAGE_HEIGHT ()/2 - disp_height/2);

              pic_actors = g_list_append (pic_actors, actor);
#ifdef HAVE_GAMMA
              new_black_actor(actor, group);
#endif
              i++;
              printf ("\n");
            }
          else
            printf ("failed\n");
          g_object_unref (file);
       }
      g_object_unref (file_info);
      file_info = g_file_enumerator_next_file (file_enumerator, NULL, NULL);
    }
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

int
main (int argc, char **argv)
{
  ClutterActor *stage, *viewport, *group;
  ClutterColor stage_color = { 0x34, 0x39, 0x39, 0xff };

  clutter_init (&argc, &argv);

  stage = clutter_stage_get_default ();

  clutter_stage_set_color (CLUTTER_STAGE (stage), &stage_color);
  //  clutter_stage_set_use_fog (CLUTTER_STAGE (stage), TRUE);
  clutter_actor_set_size (stage, 1280, 800);
  clutter_stage_fullscreen (CLUTTER_STAGE(stage));
  viewport = tidy_viewport_new ();
  g_viewport = viewport;

  clutter_actor_set_clip (viewport, 0, 0, CLUTTER_STAGE_WIDTH(), CLUTTER_STAGE_HEIGHT());

  pic_group = group = tidy_depth_group_new ();
  //  add_rects (stage, group);
  add_pics (stage, group, argc > 1 ? argv[1] : EOM_SAMPLE_DIR);
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
  timeline = clutter_timeline_new (FRAMES, 60);
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
  clutter_gesture_set_gesture_mask(gesture, stage, GESTURE_MASK_SLIDE);
  g_signal_connect (gesture, "gesture-slide-event", G_CALLBACK (gesture_cb), (gpointer)0x11223344);
#endif
  clutter_main ();

  return 0;
}

