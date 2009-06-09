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
 * Written by: Roger WANG <roger.wang@intel.com>
 * with some code from clutter-box2d example by
 *   Emmanuele Bassi <ebassi@linux.intel.com>
 *   Øyvind Kolås <pippin@linux.intel.com>
 */

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>
#include <gio/gio.h>
#include <clutter/clutter.h>
#include <clutter-gst/clutter-gst.h>
#include <clutter-box2d.h>
#include "config.h"
#include "blockbox.h"
#include "actor-manipulator.h"

#ifdef HAVE_GESTURE
#include <clutter-gesture/clutter-gesture.h>
#endif

static ClutterActor* group_;
static gint total_pics;
static GList* pic_actors;
static GHashTable* animations;

#define RECT_W 80
#define MARGIN 2

ClutterActor*
scene_get_group()
{
  return group_;
}

static void
size_change (ClutterTexture *texture,
	     gint            width,
	     gint            height,
	     gpointer        user_data)
{
  gint           new_x, new_y, new_width, new_height;

  new_height = ( height * CLUTTER_STAGE_WIDTH() ) / width;
  if (new_height <= CLUTTER_STAGE_HEIGHT())
    {
      new_width = CLUTTER_STAGE_WIDTH();

      new_x = 0;
      new_y = (CLUTTER_STAGE_HEIGHT() - new_height) / 2;
    }
  else
    {
      new_width  = ( width * CLUTTER_STAGE_HEIGHT() ) / height;
      new_height = CLUTTER_STAGE_HEIGHT();

      new_x = (CLUTTER_STAGE_WIDTH() - new_width) / 2;
      new_y = 0;
    }

  clutter_actor_set_position (CLUTTER_ACTOR (texture), new_x, new_y);

  clutter_actor_set_size (CLUTTER_ACTOR (texture),
			  new_width,
			  new_height);
}


static void
add_actor_to_cage (ClutterActor* actor, ClutterActor* group)
{
  int width, height;
  gint x, y;
  x = clutter_actor_get_width (clutter_stage_get_default ()) / 2;
  y = clutter_actor_get_height (clutter_stage_get_default ()) * 0.5;
  ClutterColor bg_color = { 0xff, 0xff, 0xff, 0xff };

  clutter_actor_get_size (actor, &width, &height);
  /* the frame of the picture */

  ClutterActor* frame = clutter_group_new();
  ClutterActor* margin = clutter_rectangle_new_with_color (&bg_color);
  clutter_actor_set_size (margin, width + MARGIN * 2, height + MARGIN * 2);
  clutter_group_add (CLUTTER_GROUP (frame), margin);
  clutter_group_add (CLUTTER_GROUP (frame), actor);
  clutter_actor_set_position (actor, MARGIN, MARGIN);

  /* setting name so the manipulator can work */
  clutter_actor_set_name (frame, "frame");
  clutter_actor_set_name (margin, "margin");
  clutter_actor_set_name (actor, "pic");

  clutter_group_add (CLUTTER_GROUP (group), frame);
  clutter_actor_set_position (frame, x, y);

  clutter_container_child_set (CLUTTER_CONTAINER (group), frame,
                               "manipulatable", TRUE,
                               "mode", CLUTTER_BOX2D_DYNAMIC, NULL);

  pic_actors = g_list_append (pic_actors, actor);
}

static void
add_pics (ClutterActor *stage, ClutterActor *group, const char* img_folder)
{
  GFileEnumerator *file_enumerator;
  GFile* root = g_file_new_for_path (img_folder);
  const char *mime_type, *name;
  ClutterActor* actor, *texture;
  int i = 0;
  GstElement      *pipeline;
  GstElement       *src;
  GstElement       *warp;
  GstElement       *colorspace;
  GstElement       *sink;
  static ClutterTimeline  *timeline = NULL;

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

      if (strstr(mime_type, "video/") == mime_type)
        {
          printf("adding %s (%s) ... ", name, mime_type);
          texture = g_object_new (CLUTTER_TYPE_TEXTURE,
                                  "sync-size",       FALSE,
                                  "disable-slicing", TRUE,
                                  NULL);
          clutter_actor_set_size (texture, RECT_W * 2, RECT_W * 2);
          /* g_signal_connect (CLUTTER_TEXTURE (texture), */
          /*                   "size-change", */
          /*                   G_CALLBACK (size_change), NULL); */

          /* Set up pipeline */
          sink = clutter_gst_video_sink_new (CLUTTER_TEXTURE (texture));
          pipeline = gst_element_factory_make ("playbin", "player");
          g_object_set (pipeline,
                        "video-sink", sink,
                        NULL);

          /* pipeline = GST_PIPELINE(gst_pipeline_new (NULL)); */

          /* src = gst_element_factory_make ("videotestsrc", NULL); */
          /* warp = gst_element_factory_make ("warptv", NULL); */
          /* colorspace = gst_element_factory_make ("ffmpegcolorspace", NULL); */

          /* gst_bin_add_many (GST_BIN (pipeline), src, warp, colorspace, sink, NULL); */
          /* gst_element_link_many (src, warp, colorspace, sink, NULL); */
          GFile* file = g_file_get_child (root, name);
          char* uri = g_file_get_uri (file);

          g_object_set (pipeline, "uri", uri, NULL);
          g_free (uri);
          g_object_unref (file);

          gst_element_set_state (GST_ELEMENT(pipeline), GST_STATE_PLAYING);

          add_actor_to_cage (texture, group);
          if (!timeline)
            {
              timeline = clutter_timeline_new (100, 30); /* num frames, fps */
              g_object_set(timeline, "loop", TRUE, NULL);
              clutter_timeline_start (timeline);
            }
          i++;
          printf("\n");
        }
      else if (strstr(mime_type, "image/") == mime_type)
        {
          GFile* file = g_file_get_child (root, name);
          const char* path = g_file_get_path (file);
          printf("adding %s (%s) ... ", name, mime_type);
          if ((actor = clutter_texture_new_from_file(path, NULL)))
            {
              /* clutter_texture_set_filter_quality (CLUTTER_TEXTURE(actor), CLUTTER_TEXTURE_QUALITY_HIGH); */
              gint width, height;
              clutter_texture_get_base_size (CLUTTER_TEXTURE(actor), &width, &height);

              gint disp_height = height * RECT_W / width;
              clutter_actor_set_size (actor, RECT_W, disp_height);
              add_actor_to_cage (actor, group);
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

static void
pic_table ()
{
  ClutterActor *stage;
  ClutterActor *group;
  ClutterVertex gravity = {0,0};
  gint          i = 42;

  stage = clutter_stage_get_default ();

  group_ = group = clutter_box2d_new ();
  add_cage (group, TRUE);
  clutter_actor_set_name (group, "cage");
  clutter_group_add (CLUTTER_GROUP (stage), group);
  clutter_box2d_set_simulating (CLUTTER_BOX2D (group), 1);

  g_object_set (group, "gravity", &gravity, NULL);
}

static void
usage(const char* self_name)
{
  printf("%s: [-h] [-f] [-r <double click radius>] [image folder]\n", self_name);
}

ClutterActor*
pick_actor(ClutterActor* stage, int x, int y, gboolean* emit)
{
  ClutterActor *actor;

  actor = clutter_stage_get_actor_at_pos (CLUTTER_STAGE (stage),
                                          x, y);

  const gchar* name = clutter_actor_get_name(actor);
  if (name && (!strcmp(name, "margin") || !strcmp(name, "pic")))
    /* we should pick the outer frame here */
    actor = clutter_actor_get_parent (actor);
  else if (actor == stage ||
           CLUTTER_IS_GROUP (actor) && name && strcmp(name, "frame"))
    {
      *emit = TRUE;
      return NULL;
    }

  if (actor == NULL)
    {
      *emit = FALSE;
      return NULL;
    }
  return actor;
}

#ifdef HAVE_GESTURE

static gboolean
gesture_cb (ClutterGesture    *gesture,
            ClutterGestureEvent    *event,
            gpointer         data)
{
  ClutterActor* stage = CLUTTER_ACTOR(data);
  const char *slide_dir_name [] = {
    "dummy",
    "SLIDE_UP", "SLIDE_DOWN", "SLIDE_LEFT", "SLIDE_RIGHT"};

  printf("gesture_cb: event pointer %p\n", event);
  if (event && event->type == GESTURE_SLIDE)
    {
      ClutterGestureSlideEvent *slide = (ClutterGestureSlideEvent *)event;
      printf("slide direction :%s\n", slide_dir_name[slide->direction]);

      ClutterActor* actor = NULL;
      gboolean emit;
      actor = pick_actor (stage, slide->x_start, slide->y_start, &emit);
      if (!actor)
        return emit;
      if (g_hash_table_lookup (animations, actor) != NULL)
        return TRUE;
      /* yes, now we can do the rotation */
      ClutterTimeline *timeline = clutter_timeline_new (60, 60);
      g_hash_table_insert (animations, actor, timeline);
      clutter_timeline_set_loop (timeline, TRUE);
      ClutterBehaviour* r_behave;
      switch (slide->direction)
        {
        case SLIDE_LEFT: case SLIDE_RIGHT:
          r_behave =
            clutter_behaviour_rotate_new (clutter_alpha_new_full (timeline,
                                                                  CLUTTER_LINEAR),
                                          CLUTTER_Y_AXIS,
                                          CLUTTER_ROTATE_CW,
                                          0, 360);
          int width = clutter_actor_get_width (actor);
          g_object_set (G_OBJECT (r_behave), "center-x", width/2, NULL);
          break;
        case SLIDE_UP: case SLIDE_DOWN:
          r_behave =
            clutter_behaviour_rotate_new (clutter_alpha_new_full (timeline,
                                                                  CLUTTER_LINEAR),
                                          CLUTTER_X_AXIS,
                                          CLUTTER_ROTATE_CW,
                                          0, 360);
          int height = clutter_actor_get_height (actor);
          g_object_set (G_OBJECT (r_behave), "center-y", height/2, NULL);
          break;
        default:
          g_error("ERROR: this should not happen!");
          return TRUE;
          break;
        }

      clutter_behaviour_apply (r_behave, actor);
      clutter_timeline_start (timeline);
    }

  return TRUE;
}

static void
scale_normal(ClutterTimeline *timeline, ClutterActor* actor)
{
  if (actor)
    clutter_actor_animate (actor, CLUTTER_LINEAR, 1000,
                          "scale-x", 1.0, "scale-y", 1.0, NULL);
}

static void
scale_back(ClutterTimeline *timeline, ClutterActor* actor)
{
  if (actor)
    {
      ClutterAnimation* view_ani = clutter_actor_animate (actor, CLUTTER_LINEAR, 3000,
                                                          "scale-x", 3.0, "scale-y", 3.0, NULL);
      ClutterTimeline* timeline = clutter_animation_get_timeline (view_ani);
      g_signal_connect (timeline, "completed", G_CALLBACK(scale_normal), actor);
    }
}

static gboolean
button_press (ClutterActor *stage,
              ClutterEvent *event,
              gpointer      data)
{
  ClutterActor* actor = NULL;
  gboolean emit;
  actor = pick_actor (stage, event->button.x, event->button.y, &emit);
  if (!actor)
    return emit;
  ClutterTimeline* timeline = (ClutterTimeline*)g_hash_table_lookup (animations, actor);
  if (timeline)
    {
      clutter_timeline_stop (timeline);
      int width, height;
      clutter_actor_get_size (actor, &width, &height);
      clutter_actor_set_rotation (actor, CLUTTER_X_AXIS, 0, 0, height / 2, 0);
      clutter_actor_set_rotation (actor, CLUTTER_Y_AXIS, 0, width / 2, 0, 0);
      g_object_unref (timeline);
      g_hash_table_remove (animations, actor);
    }
  else if (event->button.click_count == 2)
    {
      ClutterAnimation* view_ani = clutter_actor_animate (actor, CLUTTER_EASE_OUT_BACK, 1000,
                                                          "scale-x", 3.0, "scale-y", 3.0, NULL);
      ClutterTimeline* timeline = clutter_animation_get_timeline (view_ani);
      g_signal_connect (timeline, "completed", G_CALLBACK(scale_back), actor);
    }

  return FALSE;
}

#endif

static void
stage_key_press_event_cb (ClutterActor *actor, ClutterKeyEvent *event,
                          gpointer data)
{
  switch (event->keyval)
    {
    case CLUTTER_Escape :
      clutter_main_quit();
      break;
    }
}


gint
main (int   argc,
      char *argv[])
{
  ClutterActor *stage;
  ClutterColor  stage_color = { 0x00, 0x00, 0x00, 0x00 };
  gboolean hide_cursor = FALSE;
  gboolean fullscreen = FALSE;
  int double_click_radius = 10, c;

  clutter_init (&argc, &argv);
  gst_init (&argc, &argv);

  while ((c = getopt (argc, argv, "fhr:")) != -1)
    switch (c)
      {
      case 'h':
        hide_cursor = TRUE;
        break;
      case 'f':
        fullscreen = TRUE;
        break;
      case 'r':
        double_click_radius = atoi(optarg);
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

  stage = clutter_stage_get_default ();
  clutter_stage_set_color (CLUTTER_STAGE (stage), &stage_color);

  if (fullscreen)
    clutter_stage_fullscreen (CLUTTER_STAGE(stage));
  else
    clutter_actor_set_size (stage, 720, 576);

  if (hide_cursor)
    clutter_stage_hide_cursor (CLUTTER_STAGE(stage));

  printf ("SETTING DOUBLE CLICK DISTANCE = %d\n", double_click_radius);
  clutter_backend_set_double_click_distance (clutter_get_default_backend (),
                                             double_click_radius);

  const char* folder;
  if (optind < argc)
    folder = argv[optind];
  else
    folder = EOM_SAMPLE_DIR;
  printf ("LOADING FROM %s\n", folder);


  actor_manipulator_init (stage);

  clutter_actor_show (stage);

  animations = g_hash_table_new (g_direct_hash, g_direct_equal);
  pic_table();
  add_pics (stage, group_, folder);

#ifdef HAVE_GESTURE
  ClutterGesture *gesture;

  gesture = clutter_gesture_new(CLUTTER_ACTOR(stage));
  clutter_gesture_set_gesture_mask(gesture, stage, GESTURE_MASK_SLIDE);
  g_signal_connect (gesture, "gesture-slide-event", G_CALLBACK (gesture_cb), (gpointer)stage);
  g_signal_connect (stage, "button-press-event",
                    G_CALLBACK (button_press), NULL);
#endif
  g_signal_connect (stage, "key-press-event",
                    G_CALLBACK (stage_key_press_event_cb), NULL);

  clutter_main ();

  return EXIT_SUCCESS;
}


