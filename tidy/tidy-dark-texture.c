/* tidy-dark-texture.c */

#include "tidy-dark-texture.h"

G_DEFINE_TYPE (TidyDarkTexture, tidy_dark_texture, CLUTTER_TYPE_TEXTURE)

#define DARK_TEXTURE_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), TIDY_TYPE_DARK_TEXTURE, TidyDarkTexturePrivate))

struct _TidyDarkTexturePrivate
{
  guint gamma; /* 0 - 255 */
};

enum
  {
    PROP_0,

    PROP_GAMMA
  };

static void
tidy_dark_texture_paint (ClutterActor *actor)
{
  CoglHandle material;
  ClutterActorBox box;
  TidyDarkTexturePrivate* priv = TIDY_DARK_TEXTURE (actor)->priv;

  clutter_actor_get_allocation_box (actor, &box);

  g_object_get (actor, "cogl-material", &material, NULL);

  cogl_material_set_color4ub (material, priv->gamma, priv->gamma, priv->gamma,
                              clutter_actor_get_paint_opacity (actor));
  cogl_set_source (material);
  cogl_rectangle (0, 0, (box.x2 - box.x1), (box.y2 - box.y1));
}

static void
tidy_dark_texture_get_property (GObject *object, guint property_id,
                              GValue *value, GParamSpec *pspec)
{
  TidyDarkTexturePrivate* priv = TIDY_DARK_TEXTURE (object)->priv;

  switch (property_id)
    {
    case PROP_GAMMA:
      g_value_set_uint (value, priv->gamma);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
tidy_dark_texture_set_property (GObject *object, guint property_id,
                              const GValue *value, GParamSpec *pspec)
{
  TidyDarkTexturePrivate* priv = TIDY_DARK_TEXTURE (object)->priv;

  switch (property_id)
    {
    case PROP_GAMMA:
      priv->gamma = g_value_get_uint (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
tidy_dark_texture_class_init (TidyDarkTextureClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  ClutterActorClass *actor_class = CLUTTER_ACTOR_CLASS (klass);

  g_type_class_add_private (klass, sizeof (TidyDarkTexturePrivate));

  object_class->get_property = tidy_dark_texture_get_property;
  object_class->set_property = tidy_dark_texture_set_property;

  actor_class->paint = tidy_dark_texture_paint;

  g_object_class_install_property (object_class,
                                   PROP_GAMMA,
                                   g_param_spec_uint ("gamma",
                                                      "Gamma",
                                                      "Gamma value of the texture",
                                                      0, 255,
                                                      255,
                                                      G_PARAM_READWRITE));

}

static void
tidy_dark_texture_init (TidyDarkTexture *self)
{
  self->priv = DARK_TEXTURE_PRIVATE (self);
}

TidyDarkTexture *
tidy_dark_texture_new (void)
{
  return g_object_new (TIDY_TYPE_DARK_TEXTURE, NULL);
}

ClutterActor*
tidy_dark_texture_new_from_file (const gchar *filename,
                                 GError     **error)
{
  ClutterActor *texture = (ClutterActor*) tidy_dark_texture_new ();

  if (!clutter_texture_set_from_file (CLUTTER_TEXTURE (texture),
				      filename, error))
    {
      g_object_ref_sink (texture);
      g_object_unref (texture);

      return NULL;
    }
  else
    return texture;
}
