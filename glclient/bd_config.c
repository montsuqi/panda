/*	PANDA -- a simple transaction monitor

Copyright (C) 1998-1999 Ogochan.
              2000-2003 Ogochan & JMA (Japan Medical Association).

This module is part of PANDA, written by Kouji TAKAO <kouji@netlab.jp>.

	PANDA is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY.  No author or distributor accepts responsibility
to anyone for the consequences of using it or for whether it serves
any particular purpose or works at all, unless he says so in writing.
Refer to the GNU General Public License for full details. 

	Everyone is granted permission to copy, modify and redistribute
PANDA, but only under the conditions described in the GNU General
Public License.  A copy of this license is supposed to have been given
to you along with PANDA so you can know your rights and
responsibilities.  It should be in a file named COPYING.  Among other
things, the copyright notice and this notice must be preserved on all
copies. 
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>         /* strcmp */
#include <glib.h>
#include <libxml/tree.h>
#include <libxml/parser.h>
#include <sys/types.h>		/* open, fstat, mode_t */
#include <sys/stat.h>		/* open, fstat */
#include <fcntl.h>		    /* open */
#include <unistd.h>		    /* fstat, close */
#include <libgen.h>         /* dirname, basename */

#include "bd_config.h"

#define XML_CHILDREN(node) ((node)->childs)

typedef struct _BDConfigValue BDConfigValue;

/*********************************************************************
 * BDConfigValue
 ********************************************************************/
struct _BDConfigValue {
  gchar *name;
  gchar *value;
};

static BDConfigValue *
bd_config_value_new (gchar *name, gchar *value)
{
  BDConfigValue *self;

  self = g_new (BDConfigValue, 1);
  self->name = g_strdup (name);
  self->value = g_strdup (value);

  return self;
}

static void
bd_config_value_free (BDConfigValue *self)
{
  g_free (self->name);
  g_free (self->value);
  g_free (self);
}

static void
bd_config_value_inspect (BDConfigValue *self, FILE *fp)
{
  if (fp == NULL)
    fp = stdout;
  fprintf (fp, "%s=%s\n", self->name, self->value);
}

static gboolean
bd_config_value_to_bool (BDConfigValue *self)
{
  if (strncmp (self->value, "true", sizeof ("true")) != 0)
    return FALSE;
  return TRUE;
}

#define bd_config_value_to_string(self) ((self)->value)

static void
bd_config_value_replace (BDConfigValue *self, gchar *value)
{
  g_free (self->value);
  self->value = g_strdup (value);
}

/*********************************************************************
 * BDConfigSection
 ********************************************************************/
struct _BDConfigSection {
  gchar *name;
  GHashTable *values;
  GList *names;
};

static BDConfigSection *
bd_config_section_new (gchar *name)
{
  BDConfigSection *self;

  self = g_new (BDConfigSection, 1);
  self->name = g_strdup (name);
  self->values = g_hash_table_new(g_str_hash, g_str_equal);
  self->names = NULL;

  return self;
}

static void
ghfunc_value_free (gchar *key, BDConfigValue *value, gpointer user_data)
{
  bd_config_value_free (value);
  key = NULL;
  user_data = NULL;
}

static void
bd_config_section_free (BDConfigSection *self)
{
  g_free (self->name);
  g_hash_table_foreach (self->values, (GHFunc) ghfunc_value_free, NULL);
  g_hash_table_destroy(self->values);
  g_list_free (self->names);
  g_free (self);
}

static void
ghfunc_value_inspect (gchar *key, BDConfigValue *value, FILE *fp)
{
  bd_config_value_inspect (value, fp);
  key = NULL;
}

void
bd_config_section_inspect (BDConfigSection *self, FILE *fp)
{
  if (fp == NULL)
    fp = stdout;
  fprintf (fp, "[%s]\n", self->name);
  g_hash_table_foreach (self->values, (GHFunc) ghfunc_value_inspect, fp);
}

gchar *
bd_config_section_get_name (BDConfigSection *self)
{
  return self->name;
}

gboolean
bd_config_section_get_bool (BDConfigSection *self, gchar *name)
{
  BDConfigValue *value;
  
  value = g_hash_table_lookup (self->values, name);
  return bd_config_value_to_bool (value);
}

gchar *
bd_config_section_get_string (BDConfigSection *self, gchar *name)
{
  BDConfigValue *value;
  
  value = (BDConfigValue *) g_hash_table_lookup (self->values, name);
  return bd_config_value_to_string (value);
}

gboolean
bd_config_section_set_bool (BDConfigSection *self, gchar *name, gboolean bool)
{
  return bd_config_section_set_string (self, name, bool ? "true" : "false");
}

gboolean
bd_config_section_set_string (BDConfigSection *self, gchar *name, gchar *str)
{
  BDConfigValue *value;
  
  if ((value = (BDConfigValue *) g_hash_table_lookup (self->values, name)) == NULL)
    bd_config_section_append_value (self, name, str);
  else
    bd_config_value_replace (value, str);
  
  return TRUE;
}

void
bd_config_section_append_value (BDConfigSection *self, gchar *name, gchar *contents)
{
  BDConfigValue *value;

  value = bd_config_value_new (name, contents);
  g_hash_table_insert (self->values, value->name, value);
  self->names = g_list_append (self->names, value->name);
}

/*********************************************************************
 * BDConfig
 ********************************************************************/
struct _BDConfig {
  gchar *filename;
  GHashTable *sections;
  GList *names;
};

static int
is_file (const char *filename)
{
  int fd;
  struct stat stat_buf;

  if ((fd = open (filename, O_RDONLY)) == -1)
    {
#if 0
      fprintf (stderr, "error: `%s' don't exist or could not read.\n",
	       filename);
#endif
      return 0;
    }
  fstat (fd, &stat_buf);
  if (S_ISDIR (stat_buf.st_mode))
    {
#if 0
      fprintf (stderr, "error: `%s' is directory.\n", filename);
#endif
      close (fd);
      return 0;
    }
  close (fd);
  return 1;
}

static xmlDocPtr
bd_config_file_to_dom (char *filename)
{
  xmlDocPtr doc = NULL;
  xmlNsPtr namespace;

  if (!is_file (filename))
    goto error;

  if ((doc = xmlParseFile (filename)) == NULL)
    {
      fprintf (stderr, "error: `%s' unknown file type.", filename);
      goto error;
    }

  if (doc->root == NULL)
    {
      fprintf (stderr, "error: `%s' unknown file type.", filename);
      goto error;
    }

  namespace = xmlSearchNs (doc, doc->root, "glclient");
  if (namespace == NULL || xmlStrcmp (doc->root->name, "config") != 0)
    {
      fprintf (stderr, "error: `%s' is not glclient config file.", filename);
      goto error;
    }

  return doc;

error:
  if (doc != NULL)
    xmlFreeDoc (doc);
  return NULL;
}

static void
bd_config_section_parse_xml (BDConfigSection *self, xmlNodePtr root)
{
  xmlNodePtr node;
  BDConfigValue *value;
  gchar *name;
  gchar *contents;
  
  g_return_if_fail (xmlStrcmp (root->name, "section") == 0);
  
  for (node = XML_CHILDREN (root); node != NULL; node = node->next)
    {
      if (xmlStrcmp (node->name, "entry") != 0)
        continue;
      if ((name = xmlGetProp (node, "name")) == NULL)
        continue;
      contents = xmlNodeListGetString(node->doc, node->childs, TRUE);
      if ((value = g_hash_table_lookup (self->values, name)) != NULL)
        bd_config_value_replace (value, contents);
      else
        bd_config_section_append_value (self, name,
                                        contents != NULL ? contents : "");
      free (name);
      free (contents);
    }
}

static void
bd_config_parse_file (BDConfig *self, gchar *filename)
{
  xmlDocPtr doc;
  BDConfigSection *section;
  xmlNodePtr node;
  gchar *name;
  
  if ((doc = bd_config_file_to_dom (filename)) == NULL)
    return;
  
  for (node = XML_CHILDREN (doc->root); node != NULL; node = node->next)
    {
      if (xmlStrcmp (node->name, "section") == 0)
        {
          if ((name = xmlGetProp (node, "name")) != NULL)
            {
              if ((section = g_hash_table_lookup (self->sections, name)) == NULL)
                section = bd_config_append_section (self, name);
              bd_config_section_parse_xml (section, node);
              free (name);
            }
        }
    }
  xmlFreeDoc (doc);
}

BDConfig *
bd_config_new (gchar *filename)
{
  BDConfig *self;
  
  self = g_new0 (BDConfig, 1);
  self->filename = g_strdup (filename);
  self->sections = g_hash_table_new (g_str_hash, g_str_equal);
  self->names = NULL;

  return self;
}

BDConfig *
bd_config_new_with_filename (gchar *filename)
{
  BDConfig *self;

  self = bd_config_new (filename);
  bd_config_parse_file (self, filename);
  
  return self;
}

static void
ghfunc_section_free (gchar *key, BDConfigSection *section, gpointer user_data)
{
  bd_config_section_free (section);
  key = NULL;
  user_data = NULL;
}

void
bd_config_free (BDConfig *self)
{
  g_free (self->filename);
  g_hash_table_foreach (self->sections, (GHFunc) ghfunc_section_free, NULL);
  g_hash_table_destroy (self->sections);
  g_list_free (self->names);
  g_free (self);
}

static void
ghfunc_section_inspect (gchar *key, BDConfigSection *section, FILE *fp)
{
  bd_config_section_inspect (section, fp);
  key = NULL;
}

void
bd_config_inspect (BDConfig *self, FILE *fp)
{
  if (fp == NULL)
    fp = stdout;
  g_hash_table_foreach (self->sections, (GHFunc) ghfunc_section_inspect, fp);
}

gchar *
bd_config_get_filename (BDConfig *self)
{
  return self->filename;
}

BDConfigSection *
bd_config_get_section (BDConfig *self, gchar *name)
{
  BDConfigSection *section;
  
  section = (BDConfigSection *) g_hash_table_lookup (self->sections, name);
  return section;
}

GList *
bd_config_get_sections (BDConfig *self)
{
  return self->names;
}

gboolean
bd_config_exist_section (BDConfig *self, gchar *name)
{
  return (bd_config_get_section (self, name) != NULL);
}

BDConfigSection *
bd_config_append_section (BDConfig *self, gchar *name)
{
  BDConfigSection *section;
  
  section = bd_config_section_new (name);
  g_hash_table_insert (self->sections, section->name, section);
  self->names = g_list_append (self->names, section->name);

  return section;
}

gchar *
bd_config_get_string (BDConfig * self, gchar * section_name, gchar * value_name)
{
  BDConfigSection *section;
  
  section = bd_config_get_section (self, section_name);
  return bd_config_section_get_string (section, value_name);
}

static void
bd_config_section_to_xml (BDConfigSection *self, FILE *fp)
{
  GList *p;

  for (p = self->names; p != NULL; p = g_list_next (p))
    {
      gchar *name = p->data;
      gchar *value;
      
      value = bd_config_section_get_string (self, name);
      fprintf (fp, "    <entry name=\"%s\">%s</entry>\n", name, value);
    }
}

gboolean
bd_config_save (BDConfig *self, gchar *filename, mode_t mode)
{
  FILE *fp;
  GList *p;
  
  if (filename == NULL)
    filename = self->filename;

  if (strcmp (filename, self->filename) != 0)
    {
      g_free (self->filename);
      self->filename = g_strdup (filename);
    }
  else
    {
      GString *buf = g_string_new (NULL);
      g_string_sprintf (buf, "%s~", self->filename);
      rename (self->filename, buf->str);
      g_string_free (buf, TRUE);
    }

  if ((fp = fopen (filename, "w")) == NULL)
    {
      fprintf (stderr, "error: could not open for write: %s\n", filename);
      return FALSE;
    }

  fprintf (fp, "<?xml version=\"1.0\" encoding=\"EUC-JP\"?>\n");
  fprintf (fp, "<config xmlns:glclient=\"http://www.montsuqi.org/\">\n");
  for (p = self->names; p != NULL; p = g_list_next (p))
    {
      gchar *name = p->data;
      BDConfigSection *section;
      
      section = bd_config_get_section (self, name);
      fprintf (fp, "  <section name=\"%s\">\n", section->name);
      bd_config_section_to_xml (section, fp);
      fprintf (fp, "  </section>\n");
    }
  fprintf (fp, "</config>\n");
  fclose (fp);
  
  chmod (filename, mode);

  return TRUE;
}

mode_t
bd_config_permissions (BDConfig *self)
{
  int fd;
  struct stat stat_buf;

  g_return_val_if_fail (self != NULL, 0000);

  if ((fd = open (self->filename, O_RDONLY)) == -1)
    return 0000;
  
  fstat (fd, &stat_buf);
  close (fd);

  return 0777 & stat_buf.st_mode;
}

/*************************************************************
 * Local Variables:
 * mode: c
 * c-set-style: gnu
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 ************************************************************/
