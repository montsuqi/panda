/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 1989-1999 Ogochan.
 * Copyright (C) 2000-2003 Ogochan & JMA (Japan Medical Association).
 * Copyright (C) 2004-2008 Ogochan.
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

#ifndef	_INC_MESSAGE_H
#define	_INC_MESSAGE_H

#define	MESSAGE_MESSAGE		0
#define	MESSAGE_DEBUG		1
#define	MESSAGE_WARN		2
#define	MESSAGE_ERROR		3
#define	MESSAGE_LOG			4
#define	MESSAGE_PRINT		5

#define	MessageLog(msg)				_Message(MESSAGE_LOG,__FILE__,__LINE__,(msg))
#define	MessageDebug(msg)			_Message(MESSAGE_DEBUG,__FILE__,__LINE__,(msg))
#define	MessagePrintf(fmt, ...)		_MessageLevelPrintf(MESSAGE_PRINT, __FILE__,__LINE__,(fmt), __VA_ARGS__)
#define	MessageLogPrintf(fmt, ...)	_MessageLevelPrintf(MESSAGE_LOG, __FILE__,__LINE__,(fmt), __VA_ARGS__)
#define	MessageDebugPrintf(fmt, ...)	_MessageLevelPrintf(MESSAGE_DEBUG, __FILE__,__LINE__,(fmt), __VA_ARGS__)

extern	void	_MessagePrintf(char *file, int line, char *format, ...);
extern	void	_MessageLevelPrintf(int level, char *file, int line, char *format, ...);
extern	void	_Message(int level, char *file, int line, char *msg);
extern	void	__Message(int level, char *file, int line, char *msg);
extern	void	InitMessage(char *id,char *fn);
extern	void	SetMessageFunction(void (*func)(int level, char *file, int line, char *msg));

#undef	GLOBAL
#ifdef	MAIN
#define	GLOBAL		/*	*/
#else
#define	GLOBAL		extern
#endif


#undef	GLOBAL

#endif
