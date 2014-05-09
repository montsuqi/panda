#if 0
#!/bin/bash
src=$0
obj=${src%.*}
gcc -g -Wl,--no-as-needed `pkg-config --cflags --libs libglade-panda-2.0` -o $obj $src
$obj
exit
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include <gtkpanda/gtkpanda.h>
#include <glade/glade.h>

#define DEFAULT_CHAR_SIZE 256
#define DEFAULT_ARRAY_SIZE 64

void
putLevel(int l)
{
  int i;
  for(i=0;i<l;i++) {
    printf("  ");
  }
}

void
GenRecord(
  GtkWidget *widget,
  int level)
{
  long type;
  int i,length,len,slen;
  char *format,*p;
  gboolean haveDot;

  putLevel(level);
  printf("%s { #%s\n",
   glade_get_widget_name(widget),G_OBJECT_TYPE_NAME(widget));

  type = (long)(G_OBJECT_TYPE(widget));
  if (type == GTK_TYPE_NOTEBOOK) {
    putLevel(level+1);
    printf("pageno int;\n");
  } else if (type == GTK_TYPE_WINDOW) {
    putLevel(level+1);
    printf("title varchar(%d);\n",DEFAULT_CHAR_SIZE);
    putLevel(level+1);
    printf("popup_summary varchar(%d);\n",DEFAULT_CHAR_SIZE);
    putLevel(level+1);
    printf("popup_body varchar(%d);\n",DEFAULT_CHAR_SIZE);
    putLevel(level+1);
    printf("popup_icon varchar(%d);\n",DEFAULT_CHAR_SIZE);
    putLevel(level+1);
    printf("popup_timeout int;\n",DEFAULT_CHAR_SIZE);
  } else if (type == GTK_PANDA_TYPE_CLIST) {
    putLevel(level+1);
    printf("count int;\n");
    putLevel(level+1);
    printf("row int;\n");
    putLevel(level+1);
    printf("selectdata bool[%d];\n",DEFAULT_ARRAY_SIZE);
    putLevel(level+1);
    printf("fgcolor varchar(%d)[%d];\n",DEFAULT_CHAR_SIZE,DEFAULT_ARRAY_SIZE);
    putLevel(level+1);
    printf("bgcolor varchar(%d)[%d];\n",DEFAULT_CHAR_SIZE,DEFAULT_ARRAY_SIZE);
    putLevel(level+1);
    printf("item {\n");
    for(i=0;i<gtk_panda_clist_get_columns(GTK_PANDA_CLIST(widget));i++) {
      putLevel(level+2);
      printf("column%d varchar(%d);\n",i+1,DEFAULT_CHAR_SIZE);
    }
    putLevel(level+1);
    printf("}[%d];\n",DEFAULT_ARRAY_SIZE);
  } else if (type == GTK_PANDA_TYPE_TABLE) {
    putLevel(level+1);
    printf("state int;\n");
    putLevel(level+1);
    printf("style varchar(%d);\n",DEFAULT_CHAR_SIZE);
    putLevel(level+1);
    printf("trow int;\n");
    putLevel(level+1);
    printf("trowattr int;\n");
    putLevel(level+1);
    printf("tcolumn int;\n");
    putLevel(level+1);
    printf("tvalue varchar(%d);\n",DEFAULT_CHAR_SIZE);
    putLevel(level+1);
    printf("rowdata {\n");
    for(i=0;i<gtk_panda_table_get_columns(GTK_PANDA_TABLE(widget));i++) {
      putLevel(level+2);
      printf("column%d {\n",i+1);
        putLevel(level+3);
        printf("celldata varchar(%d);\n",DEFAULT_CHAR_SIZE);
        putLevel(level+3);
        printf("fgcolor varchar(%d);\n",DEFAULT_CHAR_SIZE);
        putLevel(level+3);
        printf("bgcolor varchar(%d);\n",DEFAULT_CHAR_SIZE);
      putLevel(level+2);
      printf("};\n\n",i+1);
    }
    putLevel(level+1);
    printf("};\n");
  } else if (type == GTK_PANDA_TYPE_COMBO) {
    putLevel(level+1);
    printf("count int;\n");
    putLevel(level+1);
    printf("item varchar(%d)[%d];\n",DEFAULT_CHAR_SIZE,DEFAULT_ARRAY_SIZE);
  } else if (type == GTK_TYPE_LABEL) {
    putLevel(level+1);
    printf("textdata varchar(%d);\n",DEFAULT_CHAR_SIZE);
  } else if (type == GTK_PANDA_TYPE_ENTRY) {
    length = gtk_entry_get_max_length(GTK_ENTRY(widget));
    if (length <= 0) {
      length = DEFAULT_CHAR_SIZE;
    }
    putLevel(level+1);
    printf("textdata varchar(%d);\n",length);
  } else if (type == GTK_PANDA_TYPE_TEXT) {
    putLevel(level+1);
    printf("textdata varchar(%d);\n",DEFAULT_CHAR_SIZE);
  } else if (type == GTK_TYPE_NUMBER_ENTRY) {
    g_object_get(G_OBJECT(widget),"format",&format,NULL);
    len = slen = 0;
    haveDot = FALSE;
    for(p = format; (p != NULL && *p != 0); p++) {
      if (*p == ',') {
        continue;
      }
      if (*p == '.') {
        haveDot = TRUE;
        continue;
      }
      len++;
      if (haveDot) {
        slen++;
      }
    }
    putLevel(level+1);
    if (slen != 0) {
      printf("numdata number(%d,%d);\n",len,slen);
    } else {
      printf("numdata number(%d);\n",len);
    }
  } else if (
    type == GTK_TYPE_TOGGLE_BUTTON ||
    type == GTK_TYPE_CHECK_BUTTON ||
    type == GTK_TYPE_RADIO_BUTTON
  ) {
    putLevel(level+1);
    printf("isactive bool;\n");
    putLevel(level+1);
    printf("label varchar(%d);\n",DEFAULT_CHAR_SIZE);
  } else if (type == GTK_TYPE_BUTTON) {
    putLevel(level+1);
    printf("state int;\n");
  } else if (type == GTK_TYPE_PROGRESS_BAR) {
    putLevel(level+1);
    printf("value int;\n");
  } else if (type == GTK_TYPE_CALENDAR) {
    putLevel(level+1);
    printf("year int;\n");
    putLevel(level+1);
    printf("month int;\n");
    putLevel(level+1);
    printf("day int;\n");
  } else if (
    type == GTK_PANDA_TYPE_PDF ||
    type == GTK_PANDA_TYPE_PIXMAP
  ) {
    putLevel(level+1);
    printf("objectdata object;\n");
  } else if (type == GTK_PANDA_TYPE_DOWNLOAD) {
    putLevel(level+1);
    printf("objectdata object;\n");
    putLevel(level+1);
    printf("filename varchar(%d);\n",DEFAULT_CHAR_SIZE);
    putLevel(level+1);
    printf("description varchar(%d);\n",DEFAULT_CHAR_SIZE);
  } else if (type == GTK_PANDA_TYPE_DOWNLOAD2) {
    putLevel(level+1);
    printf("item {\n");
      putLevel(level+2);
      printf("path varchar(%d)\n",DEFAULT_CHAR_SIZE);
      putLevel(level+2);
      printf("filename varchar(%d)\n",DEFAULT_CHAR_SIZE);
      putLevel(level+2);
      printf("description varchar(%d)\n",DEFAULT_CHAR_SIZE);
      putLevel(level+2);
      printf("nretry int\n");
    putLevel(level+1);
    printf("}[%d];\n",DEFAULT_ARRAY_SIZE);
  } else if (type == GTK_PANDA_TYPE_PRINT) {
    putLevel(level+1);
    printf("item {\n");
      putLevel(level+2);
      printf("path varchar(%d)\n",DEFAULT_CHAR_SIZE);
      putLevel(level+2);
      printf("title varchar(%d)\n",DEFAULT_CHAR_SIZE);
      putLevel(level+2);
      printf("nretry int\n");
      putLevel(level+2);
      printf("showdialog int\n");
    putLevel(level+1);
    printf("}[%d];\n",DEFAULT_ARRAY_SIZE);
  } else if (type == GTK_TYPE_FILE_CHOOSER_BUTTON) {
    putLevel(level+1);
    printf("objectdata object;\n");
    putLevel(level+1);
    printf("filename varchar(%d);\n",DEFAULT_CHAR_SIZE);
  } else if (type == GTK_TYPE_COLOR_BUTTON) {
    putLevel(level+1);
    printf("color varchar(%d);\n",DEFAULT_CHAR_SIZE);
  }
}

void
GenRecordEnd(
  int l)
{
  putLevel(l);
  printf("};\n");
}

void
GenRecordDefine(
  GtkWidget *widget,
  gpointer data)
{
  GtkWidget *child;
  int level;
  long type;

  level = GPOINTER_TO_INT(data);
  type = (long)(G_OBJECT_TYPE(widget));

  if (
    type == GTK_TYPE_HSEPARATOR||
    type == GTK_TYPE_VSEPARATOR
  ) {
    return;
  }

  if (!glade_get_widget_name(widget)) {
    return;
  }

  if (GTK_IS_CONTAINER(widget)) {
    GenRecord(widget,level);

    gtk_container_forall(GTK_CONTAINER(widget),
      GenRecordDefine,
      GINT_TO_POINTER(level+1));

    if (GTK_IS_WINDOW(widget)) {
      child = (GtkWidget *)g_object_get_data(G_OBJECT(widget), "child");
      if (child) {
        GenRecordDefine(child,GINT_TO_POINTER(level+1));
      }
    }

    GenRecordEnd(level);
  } else {
    GenRecord(widget,level);
    GenRecordEnd(level);
  }
}

int
main(int argc,char *argv[])
{
  GladeXML *xml;
  GtkWidget *root;
  GRegex *re;
  GMatchInfo *m;
  char *win;

  if (argc < 2) {
    fprintf(stderr,"./recdefgen window.glade\n");
    exit(-1);
  }

  win = NULL;
  re = g_regex_new("\\/([a-zA-Z0-9_-]+).glade",G_REGEX_CASELESS,0,NULL);
  if (g_regex_match(re,argv[1],0,&m)) {
    win = g_match_info_fetch(m,1);
    g_match_info_free(m);
  }
  g_regex_unref(re);

  gtk_init(&argc,&argv);
  gtk_panda_init(&argc,&argv);
  glade_init();
  xml = glade_xml_new(argv[1],win);

  if (!xml) {
    g_error("glade_xml_new error:%s",argv[1]);
  }

  root = glade_xml_get_widget(xml,win); 

  if (!root) {
    g_error("cant get root widget:%s",win);
  }

  GenRecordDefine(root,GINT_TO_POINTER(0));  

  return 0;
}
