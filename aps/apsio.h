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

#ifndef	_APS_H
#define	_APS_H
#include	"const.h"
#include	"enum.h"
#include	"libmondai.h"
#include	"struct.h"
#include	"net.h"

#undef	GLOBAL
#ifdef	MAIN
#define	GLOBAL		/*	*/
#else
#define	GLOBAL		extern
#endif

GLOBAL	int		ApsId;


extern	void	InitAPSIO(NETFILE *fpWFC);
extern	Bool	GetWFC(NETFILE *fpWFC, ProcessNode *node);
extern	void	PutWFC(NETFILE *fpWFC, ProcessNode *node);

#endif
