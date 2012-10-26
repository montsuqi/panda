/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 2004-2008 Kouji TAKAO & JMA (Japan Medical Association).
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>         /* strcmp */
#include <glib.h>
#include <errno.h>       /* errno */
#include <sys/stat.h>     /* mkdir */
#include <sys/types.h>     /* mkdir */
#include <libxml/tree.h>
#include <libxml/parser.h>
#include <sys/types.h>    /* open, fstat, mode_t */
#include <sys/stat.h>    /* open, fstat */
#include <fcntl.h>        /* open */
#include <unistd.h>        /* fstat, close */
#include <libgen.h>         /* dirname, basename */

#include "glclient.h"
#include "gettext.h"
#include "bd_config.h"
#include "message.h"
#include "debug.h"

typedef struct _BDConfigValue BDConfigValue;

GString *
get_path(const gchar *str, GString *path)
{
  char *p;
  p = (char *)str;
  if (str[0] == '~') {
    p ++;
    if (str[1] == '/') {
      p ++;
    }
    g_string_sprintf (path, "%s/%s", g_get_home_dir (), p);
  } else {
    g_string_sprintf (path, "%s", str);
  }
  return path;
}

/*********************************************************************
 * BDConfigValue
 ********************************************************************/
struct _BDConfigValue {
  gchar *name;
  gchar *value;
};

static BDConfigValue *
bd_config_value_new (gchar *name, const gchar *value)
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
bd_config_value_replace (BDConfigValue *self, const gchar *value)
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

extern  void
bd_config_section_inspect (BDConfigSection *self, FILE *fp)
{
  if (fp == NULL)
    fp = stdout;
  fprintf (fp, "[%s]\n", self->name);
  g_hash_table_foreach (self->values, (GHFunc) ghfunc_value_inspect, fp);
}

extern  gchar   *
bd_config_section_get_name (BDConfigSection *self)
{
  return self->name;
}

extern  gboolean
bd_config_section_get_bool (BDConfigSection *self, gchar *name)
{
  BDConfigValue *value;
  
  value = g_hash_table_lookup (self->values, name);
  if (value){
    return bd_config_value_to_bool (value);
  } else {
    return FALSE;
  }
}

extern  gboolean
bd_config_section_get_bool_default (BDConfigSection *self, 
  gchar *name,
  gboolean dvalue)
{
  BDConfigValue *value;
  
  value = g_hash_table_lookup (self->values, name);
  if (value){
    return bd_config_value_to_bool (value);
  } else {
    return dvalue;
  }
}

extern  gchar   *
bd_config_section_get_string (BDConfigSection *self, gchar *name)
{
  BDConfigValue *value;
  
  value = (BDConfigValue *) g_hash_table_lookup (self->values, name);
  if (value){
    return bd_config_value_to_string (value);
  } else {
    return "";
  }
}

extern  gchar   *
bd_config_section_get_string_default (BDConfigSection *self, 
  gchar *name,
  gchar *dvalue)
{
  BDConfigValue *value;
  
  value = (BDConfigValue *) g_hash_table_lookup (self->values, name);
  if (value){
    return bd_config_value_to_string (value);
  } else {
    return dvalue;
  }
}

extern  gboolean
bd_config_section_set_path (BDConfigSection *self, gchar *name, const gchar *str)
{
  GString *path = g_string_new (NULL);
  gboolean ret;
  path = get_path(str, path);
  ret = bd_config_section_set_string (self, name, path->str);
  g_string_free (path, TRUE);
  return ret;
}

extern  gboolean
bd_config_section_set_bool (BDConfigSection *self, gchar *name, gboolean bool)
{
  return bd_config_section_set_string (self, name, bool ? "true" : "false");
}

extern  gboolean
bd_config_section_set_string (BDConfigSection *self, gchar *name, const gchar *str)
{
  BDConfigValue *value;
  
  if ((value = (BDConfigValue *) g_hash_table_lookup (self->values, name)) == NULL)
    bd_config_section_append_value (self, name, str);
  else
    bd_config_value_replace (value, str);
  
  return TRUE;
}

extern  void
bd_config_section_append_value (BDConfigSection *self, gchar *name, const gchar *contents)
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
      fprintf (stderr, _("error: `%s' don't exist or could not read.\n"),
               filename);
#endif
      return 0;
    }
  fstat (fd, &stat_buf);
  if (S_ISDIR (stat_buf.st_mode))
    {
#if 0
      fprintf (stderr, _("error: `%s' is directory.\n"), filename);
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
  xmlNodePtr root;

  if (!is_file (filename))
    goto error;

  doc = xmlReadFile (filename, NULL, XML_PARSE_NOBLANKS|XML_PARSE_NOENT);
  if (doc == NULL)
    {
      fprintf (stderr, _("error: `%s' unknown file type."), filename);
      goto error;
    }

  root = xmlDocGetRootElement(doc);
  if (root == NULL)
    {
      fprintf (stderr, _("error: `%s' unknown file type."), filename);
      goto error;
    }

  namespace = xmlSearchNs (doc, root, "glclient");
  if (namespace == NULL || xmlStrcmp (root->name, "config") != 0)
    {
      fprintf (stderr, _("error: `%s' is not glclient config file."), filename);
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

  if (root->children == NULL) {
    fprintf(stderr,"can't find entry element\n");
    return;
  }
  
  for (node = root->children; node != NULL; node = node->next)
    {
      if (xmlStrcmp (node->name, "entry") != 0)
        continue;
      if ((name = xmlGetProp (node, "name")) == NULL)
        continue;
      contents = xmlNodeListGetString(node->doc, node->children, TRUE);
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
  xmlNodePtr root;
  gchar *name;
  
  if ((doc = bd_config_file_to_dom (filename)) == NULL)
    return;
  
  root = xmlDocGetRootElement(doc);
  if (root->children == NULL) {
    fprintf(stderr,"can't find config element\n");
    return;
  }
  for (node = root->children; node != NULL; node = node->next)
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
      fprintf (stderr, _("error: could not open for write: %s\n"), filename);
      return FALSE;
    }

  fprintf (fp, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
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

gboolean
bd_config_remove_section (BDConfig * self, gchar * name)
{
  BDConfigSection *section;
  
  if (!bd_config_exist_section (self, name))
    return FALSE;

  section = bd_config_get_section (self, name);
  g_hash_table_remove (self->sections, section->name);
  self->names = g_list_remove (self->names, section->name);
  bd_config_section_free (section);
  
  return TRUE;
}

gboolean
bd_config_move_section (BDConfig * self, gchar * name, gint to)
{
  BDConfigSection *section;
  
  if (!bd_config_exist_section (self, name))
    return FALSE;

  section = bd_config_get_section (self, name);
  self->names = g_list_remove (self->names, section->name);
  self->names = g_list_insert (self->names, section->name, to);

  return TRUE;
}

typedef struct {
  gchar *type;
  gchar *name;
  gchar *value;
} GL_CONFIG_ENTRY;

static GL_CONFIG_ENTRY conf_entries[] = {
{"string", "description",  "default"},
{"string", "host",         "localhost"},
{"string", "port",         "8000"},
{"string", "application",  "panda:orca00"},
{"string", "style",        ""},
{"string", "gtkrc",        ""},
{"string", "fontname",     "Takaoゴシック 10"},
{"bool",   "mlog",         "F"},
{"bool",   "keybuf",       "F"},
{"bool",   "im_kana_off",  "F"},
{"string", "user",         "ormaster"},
{"bool",   "savepassword", "T"},
{"string", "password",     ""},
{"bool",   "timer",        "T"},
{"string", "timerperiod",  "1000"},
#ifdef USE_SSL
{"bool",   "ssl",          "F"},
{"string", "CAfile",       ""},
{"string", "cert",         ""},
{"string", "ciphers",      "ALL:!ADH:!LOW:!MD5:!SSLv2:@STRENGTH"},
#ifdef USE_PKCS11
{"bool",   "pkcs11",       "F"},
{"string", "pkcs11_lib",   ""},
{"string", "slot",         ""},
#endif
#endif
{NULL,NULL,NULL}
};

void
gl_config_set_default(gchar *serverkey)
{
  int i;
  gchar *key;

  /* set default config*/
  for(i=0;conf_entries[i].name != NULL;i++) {
    key = g_strconcat(
      serverkey,"/", 
      conf_entries[i].name,NULL);
    if (conf_entries[i].type[0] == 's') {
      gconf_client_set_string(GConfCTX,key,conf_entries[i].value,NULL);
    } else {
      gconf_client_set_bool(GConfCTX,key,
        conf_entries[i].value[0] == 'T'?TRUE:FALSE,
        NULL);
    }
    g_free(key);
  }
}

gchar *
gl_config_new_server(void)
{
  gint nid;
  gchar *serverkey;

  nid = gconf_client_get_int(GConfCTX,GL_GCONF_NEXT_ID,NULL);
  serverkey = g_strdup_printf("%s/%d",GL_GCONF_SERVERS,nid);
  gl_config_set_default(serverkey);
  gconf_client_set_int(GConfCTX,GL_GCONF_NEXT_ID,nid+1,NULL);
#if 0
  gconf_client_suggest_sync(GConfCTX,NULL);
#endif

  return serverkey;
}

void
gl_config_init()
{
  gint nid;
  gchar *lastserver;

  g_type_init();
  GConfCTX = gconf_client_get_default();

  /* set next id*/
  nid = gconf_client_get_int(GConfCTX,GL_GCONF_NEXT_ID,NULL);
  if (nid == 0) {
    gconf_client_set_int(GConfCTX,GL_GCONF_NEXT_ID,2,NULL);
  }

  /* set last server*/
  lastserver = gconf_client_get_string(GConfCTX,GL_GCONF_SERVER,NULL);
  if (lastserver == NULL) {
    gconf_client_set_string(GConfCTX,
      GL_GCONF_SERVER,GL_GCONF_DEFAULT_SERVER,NULL);
  } else {
    g_free(lastserver);
  }
}

void
gl_config_convert_config()
{
  BDConfig *config;
  BDConfigSection *section;
  GList *p;
  gchar *hostname;
  gchar *lasthost;
  gchar *file;
  gchar *sdata, *key, *serverkey;
  int i;
  gboolean bdata;

  if (gconf_client_get_bool(GConfCTX,GL_GCONF_CONF_CONVERTED,NULL)) {
    return;
  }
  gconf_client_set_bool(GConfCTX,GL_GCONF_CONF_CONVERTED,TRUE,NULL);
  
  file = g_strconcat(g_get_home_dir(), "/.glclient/glclient.conf", NULL);
  config = bd_config_new_with_filename (file);

  if (config->names == NULL) {
    gl_config_set_default(GL_GCONF_DEFAULT_SERVER);
    return;
  }
 
  lasthost = NULL; 
  for (   p = bd_config_get_sections (config); 
      p != NULL; 
      p = g_list_next (p)) 
  {
    hostname = (char *) p->data;
    if (!strcmp (hostname, "glclient")) {
      continue;
    }
    section = bd_config_get_section (config, hostname);

    if (!strcmp (hostname, "global")) { 
      /* default */
      lasthost = bd_config_section_get_string (section, "hostname");
      bd_config_section_set_string(section,"description","default");
    }
    serverkey = gl_config_new_server();
    for(i=0;conf_entries[i].name != NULL;i++) {
      key = g_strconcat(
        serverkey,"/", 
        conf_entries[i].name,NULL);
      if (conf_entries[i].type[0] == 's') {
        sdata = bd_config_section_get_string(section,conf_entries[i].name);
        gconf_client_set_string(GConfCTX,key,sdata,NULL);
      } else {
        bdata = bd_config_section_get_bool (section, conf_entries[i].name);
        gconf_client_set_bool(GConfCTX,key,bdata,NULL);
      }
      g_free(key);
    }
    if (!g_strcmp0(lasthost,hostname)) {
      gconf_client_set_string(GConfCTX,GL_GCONF_SERVER,serverkey,NULL);
    }
    g_free(serverkey);
  }
#if 0
  g_remove(file);
#endif
}

gboolean 
gl_config_exists(gchar *name)
{
  gchar *path;
  gboolean ret = FALSE;

  path = g_strconcat(GL_GCONF_SERVERS,"/",name,NULL);
  ret = gconf_client_dir_exists(GConfCTX,path,NULL);
  return ret;
}

void 
gl_config_remove_server(gchar *serverkey)
{
  GConfUnsetFlags flags = 0;
  
  gconf_client_recursive_unset(GConfCTX,serverkey,flags,NULL);
#if 0
  gconf_client_suggest_sync(GConfCTX,NULL);
#endif
}

void 
gl_config_set_server(gchar *serverkey)
{
  gconf_client_set_string(GConfCTX,GL_GCONF_SERVER,serverkey,NULL);
#if 0
  gconf_client_suggest_sync(GConfCTX,NULL);
#endif
}

gchar* 
gl_config_get_server()
{
  gchar *server;

  server = gconf_client_get_string(GConfCTX,GL_GCONF_SERVER,NULL);
  if (!gconf_client_dir_exists(GConfCTX,server,NULL)) {
    g_free(server);
    server = g_strdup(GL_GCONF_DEFAULT_SERVER);
  }
  return server;
}

void 
gl_config_set_string(
  const gchar *serverkey,
  const gchar *key,
  const gchar *value)
{
  gchar *_key;

  _key = g_strconcat(
    serverkey,"/", 
    key,NULL);
  gconf_client_set_string(GConfCTX,_key,value,NULL);
  g_free(_key);
#if 0
  gconf_client_suggest_sync(GConfCTX,NULL);
#endif
}

gchar* 
gl_config_get_string(
  const gchar *serverkey,
  const gchar *key)
{
  gchar *_key;
  gchar *ret;

  _key = g_strconcat(
    serverkey,"/", 
    key,NULL);
  ret = gconf_client_get_string(GConfCTX,_key,NULL);
  if (ret == NULL) {
    g_strdup("");
  }
  g_free(_key);
  return ret;
}

void 
gl_config_set_bool(
  const gchar *serverkey,
  const gchar *key,
  gboolean value)
{
  gchar *_key;

  _key = g_strconcat(
    serverkey,"/", 
    key,NULL);
  gconf_client_set_bool(GConfCTX,_key,value,NULL);
  g_free(_key);
#if 0
  gconf_client_suggest_sync(GConfCTX,NULL);
#endif
}

gboolean
gl_config_get_bool(
  const gchar *serverkey,
  const gchar *key)
{
  gchar *_key;
  gboolean ret;

  _key = g_strconcat(
    serverkey,"/", 
    key,NULL);
  ret = gconf_client_get_bool(GConfCTX,_key,NULL);
  g_free(_key);
  return ret;
}

gint
gl_config_server_list_cmp(
  gconstpointer a,
  gconstpointer b)
{
  gchar *aa,*bb;

  aa = (gchar*)a;
  bb = (gchar*)b;

  if (strlen(aa) == strlen(bb)){
    return g_strcmp0(aa,bb);
  } else if (strlen(aa) < strlen(bb)) {
    return -1;
  } else {
    return 1;
  }
}

GSList *
gl_config_get_server_list()
{
  GSList *list;

  list = gconf_client_all_dirs(GConfCTX,GL_GCONF_SERVERS,NULL);
  return g_slist_sort(list,gl_config_server_list_cmp);
}

/*************************************************************
 * Local Variables:
 * mode: c
 * c-set-style: gnu
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 ************************************************************/
