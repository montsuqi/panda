/*	PANDA -- a simple transaction monitor

Copyright (C) 1989-2003 Ogochan.

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

#ifndef	_INC_MESSAGE_H
#define	_INC_MESSAGE_H

#define	MESSAGE_MESSAGE		0
#define	MESSAGE_DEBUG		1
#define	MESSAGE_WARN		2
#define	MESSAGE_ERROR		3
#define	MESSAGE_LOG			4
#define	MESSAGE_PRINT		5

#define	MessageLog(msg)				_Message(MESSAGE_LOG,__FILE__,__LINE__,(msg))
#define	MessagePrintf(fmt, ...)		_MessagePrintf(__FILE__,__LINE__,(fmt), __VA_ARGS__)

extern	void	MessageDebug(char *file, int line, char *msg);
extern	void	_MessagePrintf(char *file, int line, char *format, ...);
extern	void	_MessageLevelPrintf(int level, char *file, int line, char *format, ...);
extern	void	_Message(int level, char *file, int line, char *msg);
extern	void	InitMessage(char *id,char *fn);

#undef	GLOBAL
#ifdef	MAIN
#define	GLOBAL		/*	*/
#else
#define	GLOBAL		extern
#endif


#undef	GLOBAL

#endif
