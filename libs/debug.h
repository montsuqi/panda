/*	PANDA -- a simple transaction monitor

Copyright (C) 1989-2005 Ogochan.

This module is part of PANDA.

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

/*
*	debug macros
*/

#ifndef	_INC_DEBUG_H
#define	_INC_DEBUG_H
#include	<stdio.h>

#include	"message.h"

#ifdef	TRACE
#define	dbgmsg(s)			MessageDebug(s)
#define	dbgprintf(fmt,...)	MessageDebugPrintf((fmt), __VA_ARGS__)
#define	PASS(s)				MessageDebug(s)
#define	ENTER_FUNC			MessageDebugPrintf(">%s", __func__)
#define	LEAVE_FUNC			MessageDebugPrintf("<%s", __func__)
#define	RETURN(v)			MessageDebugPrintf("<%s", __func__),return(v)
#else
#define	dbgmsg(s)			/*	*/
#define	dbgprintf(fmt,...)	/*	*/
#define	PASS(s)				/*	*/
#define	ENTER_FUNC			/*	*/
#define	LEAVE_FUNC			/*	*/
#define	RETURN(v)			return(v)
#endif

#define	EXIT(c)	{ printf("exit at %s(%d) %s\n",__FILE__,__LINE__, __func__);exit(c);}

#ifdef	_INC_MESSAGE_H
#define	Error(...)                                                      \
do {                                                                    \
    _MessageLevelPrintf(MESSAGE_ERROR,__FILE__,__LINE__,__VA_ARGS__);   \
    exit(1);                                                            \
} while (0)
#define	Warning(...)                                                \
_MessageLevelPrintf(MESSAGE_WARN,__FILE__,__LINE__,__VA_ARGS__);
#define	Message(...)                                  \
_MessageLevelPrintf(MESSAGE_PRINT,__FILE__,__LINE__,__VA_ARGS__);
#else
#define	Error(...)                              \
do {                                            \
    printf("E:%s:%d:",__FILE__,__LINE__);       \
    printf(__VA_ARGS__);                        \
    printf("\n");                               \
    exit(1);                                    \
} while (0)
#define	Warning(...)                            \
do {                                            \
    printf("W:%s:%d:",__FILE__,__LINE__);       \
    printf(__VA_ARGS__);                        \
    printf("\n");                               \
} while (0)
#define	Message(l, ...)                         \
do {                                            \
    printf("M:%s:%d:",__FILE__,__LINE__);       \
    printf(__VA_ARGS__);                        \
    printf("\n");                               \
} while (0)
#define	MessageLog(s) printf("L:%s:%d:%s\n",__FILE__,__LINE__,(s))
#define MessageLogPrintf(...)                   \
do {                                            \
    printf("L:%s:%d:",__FILE__,__LINE__);       \
    printf(__VA_ARGS__);                        \
    printf("\n");                               \
} while (0)
#define	MessageDebug(s)	printf("D:%s:%d:%s\n",__FILE__,__LINE__,(s))
#define MessageDebugPrintf(...)                 \
do {                                            \
    printf("D:%s:%d:",__FILE__,__LINE__);       \
    printf(__VA_ARGS__);                        \
    printf("\n");                               \
} while (0)
#endif
#endif
