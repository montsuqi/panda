/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 2000-2008 Ogochan & JMA (Japan Medical Association).
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

#define	MAIN
/*
#define	DEBUG
#define	TRACE
*/

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif
#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<unistd.h>
#include	<sys/wait.h>
#include	<signal.h>

#include	"libmondai.h"
#include	"message.h"
#include	"debug.h"

static	volatile Bool exit_flag = FALSE;
extern char **environ;

void signal_handler (int signo )
{
	exit_flag = TRUE;
	printf("stop signal\n");
}

static int
registdb(
	pid_t pgid)
{
	printf("PGID=%d\n", pgid);
	printf("mcp_window:%s\n", getenv("MCP_WINDOW"));
	printf("mcp_widget:%s\n", getenv("MCP_WIDGET"));
	printf("mcp_event:%s\n", getenv("MCP_EVENT"));
	printf("mcp_tenant:%s\n", getenv("MCP_TENANT"));
	printf("mcp_term:%s\n", getenv("MCP_TERM"));
	printf("mcp_user:%s\n", getenv("MCP_USER"));
	printf("mcp_host:%s\n", getenv("MCP_HOST"));
	printf("mcp_middleware_name:%s\n", getenv("MCP_MIDDLEWARE_NAME"));
	printf("mcp_middleware_version:%s\n", getenv("MCP_MIDDLEWARE_VERSION"));
	printf("mcp_batch_name:%s\n", getenv("MCP_BATCH_NAME"));
	printf("mcp_batch_comment:%s\n", getenv("MCP_BATCH_COMMENT"));
	return 0;
}

static int
unregistdb(
	pid_t pgid)
{
	printf("finish %d\n", pgid);
	return 0;
}

extern  int
exec_shell(
	int		argc,
	char	**argv)
{
	pid_t pid, wpid;
	int i, rc = 0;
	int status;
	char *cmdv[4];
	char *sh;

	for ( i=1; i<argc; i++ ) {
		if ( ( pid = fork() ) == 0 ) {
			sh = "/bin/sh";
			cmdv[0] = sh;
			cmdv[1] = "-c";
			cmdv[2] = argv[i];
			cmdv[3] = NULL;
			execve(sh, cmdv, environ);
		} else if ( pid < 0) {
			perror("fork");
			rc = -1;
			break;
		}
		wpid = waitpid(pid, &status, 0);
		if (wpid < 0) {
			perror("waitpid");
			rc = -1;
			break;
		}
		if (WIFEXITED(status)) {
			rc = WEXITSTATUS(status);
		} else {
			rc = status;
		}
		if (exit_flag) {
			printf("signal exit\n");
			break;
		}
	}
	return rc;
}

extern	int
main(
	int		argc,
	char	**argv)
{
	int rc;
	pid_t pgid;
	struct sigaction sa;

	memset(&sa, 0, sizeof(struct sigaction));
	sa.sa_handler = signal_handler;
	sa.sa_flags |= SA_RESTART;
	if (sigaction(SIGHUP, &sa, NULL) != 0) {
		Error("sigaction(2) failure");
	}

	pgid = getpgrp();
	registdb(pgid);

	rc = exec_shell(argc, argv);
	printf("status=%d\n", rc);

	unregistdb(pgid);
	return 0;
}
