/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 2004-2008 Ogochan & JMA (Japan Medical Association).
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
#include	<string.h>
#include    <sys/types.h>
#include	<sys/stat.h>
#include	<glib.h>
#include	<pthread.h>

#include	"libmondai.h"
#include	"net.h"
#include	"comm.h"
#include	"wfcdata.h"
#include	"sessionctrl.h"
#include	"sysdatacom.h"
#include	"message.h"
#include	"debug.h"

extern	void
ServeCommon(
	NETFILE		*fp,
	PacketClass	pc)
{
	SessionCtrl *ctrl;
	LargeByteString *buf;
ENTER_FUNC;
	switch	(pc) {
	case SYSDB_GET_DATA:
		ctrl = NewSessionCtrl(SESSION_CONTROL_GET_DATA);
		break;
	case SYSDB_GET_MESSAGE:
		ctrl = NewSessionCtrl(SESSION_CONTROL_GET_MESSAGE);
		break;
	case SYSDB_RESET_MESSAGE:
		ctrl = NewSessionCtrl(SESSION_CONTROL_RESET_MESSAGE);
		break;
	case SYSDB_SET_MESSAGE:
		ctrl = NewSessionCtrl(SESSION_CONTROL_SET_MESSAGE);
		break;
	case SYSDB_SET_MESSAGE_ALL:
		ctrl = NewSessionCtrl(SESSION_CONTROL_SET_MESSAGE_ALL);
		break;
	default:
		Error("unreachable");
		break;
	}
	buf = NewLBS();
	RecvLBS(fp,buf);
		ON_IO_ERROR(fp,badio);
	NativeUnPackValue(NULL,LBS_Body(buf),ctrl->sysdbval);
	ctrl = ExecSessionCtrl(ctrl);	
    LBS_ReserveSize(buf,NativeSizeValue(NULL,ctrl->sysdbval),FALSE);
	NativePackValue(NULL,LBS_Body(buf),ctrl->sysdbval);	
	SendPacketClass(fp,ctrl->rc);
		ON_IO_ERROR(fp,badio);
	SendLBS(fp,buf);
		ON_IO_ERROR(fp,badio);
badio:
	FreeSessionCtrl(ctrl);
	FreeLBS(buf);
LEAVE_FUNC;
}

extern	void
ServeGetDataAll(
	NETFILE		*fp)
{
	SessionCtrl *ctrl;
	LargeByteString *buf;
ENTER_FUNC;
	ctrl = NewSessionCtrl(SESSION_CONTROL_GET_DATA_ALL);
	ctrl = ExecSessionCtrl(ctrl);	
	buf = NewLBS();
	LBS_ReserveSize(buf,NativeSizeValue(NULL,ctrl->sysdbvals),FALSE);
	NativePackValue(NULL,LBS_Body(buf),ctrl->sysdbvals);
	SendPacketClass(fp,ctrl->rc);
	SendInt(fp,ctrl->size);
		ON_IO_ERROR(fp,badio);
	SendLBS(fp,buf);
		ON_IO_ERROR(fp,badio);
badio:
	FreeSessionCtrl(ctrl);
	FreeLBS(buf);
LEAVE_FUNC;
}

extern	void
ServeSysDB(
	NETFILE		*fp)
{
	PacketClass pc;
ENTER_FUNC;
	pc = RecvPacketClass(fp);
			ON_IO_ERROR(fp,badio);
	switch	(pc) {
	case SYSDB_GET_DATA:
	case SYSDB_GET_MESSAGE:
	case SYSDB_RESET_MESSAGE:
	case SYSDB_SET_MESSAGE:
	case SYSDB_SET_MESSAGE_ALL:
		ServeCommon(fp,pc);
		break;
	case SYSDB_GET_DATA_ALL:
		ServeGetDataAll(fp);
		break;
	default:
		CloseNet(fp);
		break;
	}
badio:
LEAVE_FUNC;
}

