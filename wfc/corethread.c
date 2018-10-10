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

/*
#define	DEBUG
#define	TRACE
*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <glib.h>
#include <pthread.h>
#include <libmondai.h>

#include "LDparser.h"
#include "queue.h"
#include "wfcdata.h"
#include "wfc.h"
#include "termthread.h"
#include "mqthread.h"
#include "debug.h"

static Queue *CoreQueue;

extern void CoreEnqueue(SessionData *data) { EnQueue(CoreQueue, data); }

static void CoreThread(void *para) {
  SessionData *data;
  MQ_Node *mq;

  mq = NULL;
  CoreQueue = NewQueue();
  do {
    data = (SessionData *)DeQueue(CoreQueue);
    dbgmsg("de queue");
    if (data->ld == NULL) {
      MessageLogPrintf("invalid session data [%s:%s:%s]", data->hdr->uuid,
                       data->hdr->user, data->hdr->window);
      data->status = SESSION_STATUS_ABORT;
      TermEnqueue(data->term, data);
      continue;
    }
    if ((mq = g_hash_table_lookup(MQ_Hash, data->ld->info->name)) != NULL) {
      MessageEnqueue(mq, data);
    } else {
      MessageLogPrintf("LD not found [%s]", data->ld->info->name);
    }
  } while (mq != NULL);
  pthread_exit(NULL);
}

extern void StartCoreThread(void) {
  static pthread_t core;
  pthread_create(&core, NULL, (void *(*)(void *))CoreThread, NULL);
}
