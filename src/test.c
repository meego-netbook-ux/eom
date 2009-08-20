#include "config.h"

#include <string.h>
#include <gio/gio.h>

#include <clutter/clutter.h>
#include <clutter/x11/clutter-x11.h>

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

static gint total_pics;
static GList* pic_actors;

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
#ifdef HAVE_DEBUG
          printf("adding %s (%s) ... ", name, mime_type);
#endif
          if ((actor = clutter_texture_new_from_file(path, NULL)))
            {
              clutter_container_add_actor (CLUTTER_CONTAINER (stage), actor);
              clutter_actor_hide (actor);


              pic_actors = g_list_append (pic_actors, actor);

              i++;
#ifdef HAVE_DEBUG
              printf ("\n");
#endif
            }
          else
            {
#ifdef HAVE_DEBUG
              printf ("failed\n");
#endif
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
      if (argc > 2)
        {
          add_pics (stage, NULL, argv[2]);
        }
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

  clutter_main ();

  return 0;
}
