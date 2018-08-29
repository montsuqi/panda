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

#ifndef _DBREDIRECTOR_MAIN_H
#define _DBREDIRECTOR_MAIN_H

#undef GLOBAL
#ifdef MAIN
#define GLOBAL /*	*/
#else
#define GLOBAL extern
#endif

#define CONNECT_INTERVAL 60

typedef enum {
  TICKET_BEGIN = 0,
  TICKET_DATA,
  TICKET_ABORT,
  TICKET_COMMIT,
  TICKET_AUDIT
} TICKET_STATUS;

typedef struct {
  LargeByteString *checkData;
  LargeByteString *redirectData;
} VeryfyData;

typedef struct {
  int fd;
  uint64_t ticket_id;
  VeryfyData *veryfydata;
  LargeByteString *auditlog;
  TICKET_STATUS status;
} Ticket;

#endif
