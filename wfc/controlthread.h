/*	PANDA -- a simple transaction monitor

Copyright (C) 2000-2003 Ogochan & JMA (Japan Medical Association).

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

#ifndef	_INC_CONTROLTHREAD_H
#define	_INC_CONTROLTHREAD_H

#include	"wfcdata.h"

#define	WFCCONTROL_NULL		(PacketClass)0x00
#define	WFCCONTROL_OK		(PacketClass)0xE0
#define	WFCCONTROL_NG		(PacketClass)0xE1
#define	WFCCONTROL_STOP		(PacketClass)0xFE
#define	WFCCONTROL_END		(PacketClass)0xFF

extern	pthread_t	ConnectControl(int _fhControl);
extern	void		InitControl(void);

#endif
