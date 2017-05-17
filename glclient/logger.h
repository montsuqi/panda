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

#ifndef	_INC_LOGGER_H
#define	_INC_LOGGER_H

enum {
	LOG_DEBUG = 0,
	LOG_WARN,
	LOG_INFO,
	LOG_ERROR,
	LOG_LAST
};

extern void InitLogger();
extern void FinalLogger();
extern void SetErrorFunc(void (*)(char *,...));
extern void SetLogLevel(int level);
extern void logger(int level, char *file, int line, char *format, ...);
extern void Error(char *format,...);

#define	Debug(...)											\
	logger(LOG_DEBUG,__FILE__,__LINE__,__VA_ARGS__);
#define	Warning(...)										\
	logger(LOG_WARN,__FILE__,__LINE__,__VA_ARGS__);
#define	Info(...)											\
	logger(LOG_INFO,__FILE__,__LINE__,__VA_ARGS__);
#define	_Error(...)											\
	logger(LOG_ERROR,__FILE__,__LINE__,__VA_ARGS__);

#endif
