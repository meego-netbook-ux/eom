/* tidy-dark-texture.h */

#ifndef _TIDY_DARK_TEXTURE_H
#define _TIDY_DARK_TEXTURE_H

#include <glib-object.h>
#include <clutter/clutter.h>

G_BEGIN_DECLS

#define TIDY_TYPE_DARK_TEXTURE tidy_dark_texture_get_type()

#define TIDY_DARK_TEXTURE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
  TIDY_TYPE_DARK_TEXTURE, TidyDarkTexture))

#define TIDY_DARK_TEXTURE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
  TIDY_TYPE_DARK_TEXTURE, TidyDarkTextureClass))

#define TIDY_IS_DARK_TEXTURE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
  TIDY_TYPE_DARK_TEXTURE))

#define TIDY_IS_DARK_TEXTURE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
  TIDY_TYPE_DARK_TEXTURE))

#define TIDY_DARK_TEXTURE_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
  TIDY_TYPE_DARK_TEXTURE, TidyDarkTextureClass))

typedef struct _TidyDarkTexture TidyDarkTexture;
typedef struct _TidyDarkTextureClass TidyDarkTextureClass;
typedef struct _TidyDarkTexturePrivate TidyDarkTexturePrivate;

struct _TidyDarkTexture
{
  ClutterTexture parent;

  TidyDarkTexturePrivate *priv;
};

struct _TidyDarkTextureClass
{
  ClutterTextureClass parent_class;
};

GType tidy_dark_texture_get_type (void);

TidyDarkTexture *tidy_dark_texture_new (void);
ClutterActor* tidy_dark_texture_new_from_file (const gchar *filename,
                                               GError     **error);

G_END_DECLS

#endif /* _TIDY_DARK_TEXTURE_H */
