/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 1989-2008 Ogochan.
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
*	debug macros
*/

#ifndef _INC_DEBUG_H
#define _INC_DEBUG_H
#include <stdio.h>

#include "message.h"

#ifdef TRACE
#define dbgmsg(s) MessageDebug(s)
#define dbgprintf(fmt, ...) MessageDebugPrintf((fmt), __VA_ARGS__)
#define PASS(s) MessageDebug(s)
#else
#define dbgmsg(s)           /*	*/
#define dbgprintf(fmt, ...) /*	*/
#define PASS(s)             /*	*/
#endif

#define EXIT(c)                                                                \
  {                                                                            \
    printf("exit at %s(%d) %s\n", __FILE__, __LINE__, __func__);               \
    exit(c);                                                                   \
  }

#ifdef _INC_MESSAGE_H
#define Error(...)                                                             \
  do {                                                                         \
    _MessageLevelPrintf(MESSAGE_ERROR, __FILE__, __LINE__, __VA_ARGS__);       \
    exit(1);                                                                   \
  } while (0)
#define Warning(...)                                                           \
  _MessageLevelPrintf(MESSAGE_WARN, __FILE__, __LINE__, __VA_ARGS__);
#define Message(...)                                                           \
  _MessageLevelPrintf(MESSAGE_PRINT, __FILE__, __LINE__, __VA_ARGS__);
#else
#define Error(...)                                                             \
  do {                                                                         \
    printf("E:%s:%d:", __FILE__, __LINE__);                                    \
    printf(__VA_ARGS__);                                                       \
    printf("\n");                                                              \
    exit(1);                                                                   \
  } while (0)
#define Warning(...)                                                           \
  do {                                                                         \
    printf("W:%s:%d:", __FILE__, __LINE__);                                    \
    printf(__VA_ARGS__);                                                       \
    printf("\n");                                                              \
  } while (0)
#define Message(l, ...)                                                        \
  do {                                                                         \
    printf("M:%s:%d:", __FILE__, __LINE__);                                    \
    printf(__VA_ARGS__);                                                       \
    printf("\n");                                                              \
  } while (0)
#define MessageLog(s) printf("L:%s:%d:%s\n", __FILE__, __LINE__, (s))
#define MessageLogPrintf(...)                                                  \
  do {                                                                         \
    printf("L:%s:%d:", __FILE__, __LINE__);                                    \
    printf(__VA_ARGS__);                                                       \
    printf("\n");                                                              \
  } while (0)
#define MessageDebug(s) printf("D:%s:%d:%s\n", __FILE__, __LINE__, (s))
#define MessageDebugPrintf(...)                                                \
  do {                                                                         \
    printf("D:%s:%d:", __FILE__, __LINE__);                                    \
    printf(__VA_ARGS__);                                                       \
    printf("\n");                                                              \
  } while (0)
#endif
#endif
