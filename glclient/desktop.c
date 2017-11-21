/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 2009 NaCl & JMA (Japan Medical Association).
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

/*
#define	DEBUG
#define	TRACE
*/

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include	<stdio.h>
#include	<stdlib.h>
#include	<unistd.h>
#include	<fcntl.h>
#include	<sys/time.h>
#include	<sys/types.h>
#include	<sys/stat.h>
#include	<sys/wait.h>
#include	<signal.h>
#include	<errno.h>
#include	<glib.h>
#include	<libmondai.h>

#define		DESKTOP_MAIN

#include	"gettext.h"
#include	"tempdir.h"
#include	"bd_config.h"
#include	"desktop.h"
#include	"logger.h"

static char *
GetSuffix(char *path)
{
	char *p,*q;
	char *ret = NULL;

	if (path != NULL) {
 		p = path;
		while ((q = strchr(p,'.')) != NULL) {
			q++;
			ret = q;
			p = q;
		}
	}
	return ret;
}

void
InitDesktop(void)
{
	char *conf1,*conf2,*buf,*tmp;
	size_t size;
	gboolean f_read;

	DesktopAppTable = NewNameHash();
	conf1 = g_build_filename(GetRootDir(),DESKTOP_LIST,NULL);
	conf2 = g_build_filename(GLCLIENT_DATADIR,DESKTOP_LIST,NULL);
	f_read = FALSE;

	if (g_file_get_contents(conf1,&buf,&size,NULL)) {
		f_read = TRUE;
		Info("use %s\n",conf1);
	} else {
		if (g_file_get_contents(conf2,&buf,&size,NULL)) {
			f_read = TRUE;
			Info("use %s\n",conf2);
		} else {
			Warning("cannot open applications list");
		}
	}
	g_free(conf1);
	g_free(conf2);

	if (f_read) {
		tmp = realloc(buf,size+1);
		if (tmp == NULL) {
			Error("realloc(3) failure");
		} else {
			buf = tmp;
			memset(tmp+size,0,1);
		}
		{
			GRegex *reg;
			GMatchInfo *info;

			reg = g_regex_new("(.*):(.*)",0,0,NULL);
			g_regex_match(reg,buf,0,&info);
			while(g_match_info_matches(info)) {
				gchar *k,*v;
				
				k = g_match_info_fetch(info,1);
				v = g_match_info_fetch(info,2);

				Debug("app %s:%s\n",k,v);
				g_hash_table_insert(DesktopAppTable, k, v);

				g_match_info_next(info,NULL);
			}
			g_match_info_free(info);
			g_regex_unref(reg);
		}
		g_free(buf);
	}
}

extern int
CheckDesktop(char *filename)
{
	char *suffix;
	char *template;

	if (filename == NULL || strlen(filename) == 0) {
		return 0;
	}
	template = g_hash_table_lookup(DesktopAppTable, filename);
	if (template == NULL) {
		suffix = GetSuffix(filename);
		template = g_hash_table_lookup(DesktopAppTable, suffix);
    }
	if (template != NULL) {
		return 1;
	} else {
		return 0;
	}
}

static void
Exec(char *command,char *file)
{
	int argc;
	char *argv[DESKTOP_MAX_ARGC];
	char *p,*q;
	char message[SIZE_BUFF];

	argc = 0;
	p = q = command;
	while (1) {
		while (*q != ' ' && *q != 0) {q++;}
		argv[argc] = StrnDup(p, q - p);
		if (strstr(argv[argc],"%s")) {
			argv[argc] = file;
		}
		argc++;
		if (argc >= DESKTOP_MAX_ARGC) {
			Warning("can't exec [%s]; over argc size",command);
			return;
		}
		if (*q == 0) {
			argv[argc] = NULL;
			break;
		}
		while (*q == ' ') {q++;}
		p = q;
	}
	execvp(argv[0], argv);
    argv[0] = "zenity";
    argv[1] = "--error";
    argv[2] = "--title";
    argv[3] = _("Application start failure");
    argv[4] = "--text";
    sprintf(message, _("can't execute command.\\n\"%s\""),command);
    argv[5] = message;
    argv[6] = NULL;
	execvp(argv[0], argv);
	_exit(0);
}

extern void 
OpenDesktop(char *filename,LargeByteString *binary)
{
	int pid;
	int status;
	char *suffix;
	char *template;
	char path[SIZE_LONGNAME+1];

	if (filename == NULL || strlen(filename) == 0) {
		return;
	}
	if (binary == NULL || LBS_Size(binary) == 0) {
		return;
	}

	snprintf(path,SIZE_LONGNAME,"%s/%s",GetTempDir(), filename);
	path[SIZE_LONGNAME] = 0;
	if (!g_file_set_contents(path,LBS_Body(binary),LBS_Size(binary),NULL)) {
		Warning("could not create teporary file:%s",path);
		return;
	}
	template = g_hash_table_lookup(DesktopAppTable, filename);
	if (template == NULL) {
		suffix = GetSuffix(filename);
		template = g_hash_table_lookup(DesktopAppTable, suffix);
    }
	if (template != NULL) {
		if ((pid = fork()) == 0) {
			if ((pid = fork()) == 0) {
				Exec(template,path);
			} else if (pid < 0) {
				Warning("fork failure:%s",strerror(errno));
			} else {
				_exit(0);
			}
		} else if (pid < 0) {
			Warning("fork failure:%s",strerror(errno));
		} else {
			wait(&status);
		}
	}
}
