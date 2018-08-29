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

#ifndef _INC_LOGGER_H
#define _INC_LOGGER_H

#undef GLOBAL
#ifdef LOGGER_MAIN
#define GLOBAL /*	*/
#else
#define GLOBAL extern
#endif

enum { GL_LOG_DEBUG = 0, GL_LOG_WARN, GL_LOG_INFO, GL_LOG_ERROR, GL_LOG_LAST };

extern void InitLogger(const char *prefix);
extern void InitLogger_via_FileName(const char *filename);
extern void FinalLogger();
const char *GetLogFile();
extern void SetErrorFunc(void (*)(const char *, ...));
extern void SetLogLevel(int level);
extern void logger(int level, const char *file, int line, const char *format,
                   ...);
extern void Error(const char *format, ...);

#define Debug(...) logger(GL_LOG_DEBUG, __FILE__, __LINE__, __VA_ARGS__);
#define Warning(...) logger(GL_LOG_WARN, __FILE__, __LINE__, __VA_ARGS__);
#define Info(...) logger(GL_LOG_INFO, __FILE__, __LINE__, __VA_ARGS__);
#define _Error(...) logger(GL_LOG_ERROR, __FILE__, __LINE__, __VA_ARGS__);

#endif
