/*	PANDA -- a simple transaction monitor

Copyright (C) 1998-1999 Ogochan.
              2000-2003 Ogochan & JMA (Japan Medical Association).

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

#ifndef	_GLCOMM_H
#define	_GLCOMM_H
#include	"net.h"

extern	void	GL_SendPacketClass(NETFILE *fp, PacketClass c, Bool fNetwork);
extern	PacketClass	GL_RecvPacketClass(NETFILE *fp, Bool fNetwork);

extern	void	GL_SendInt(NETFILE *fp, int data, Bool fNetwork);
extern	void	GL_SendLong(NETFILE *fp, long data, Bool fNetwork);
extern	void	GL_SendString(NETFILE *fp, char *str, Bool fNetwork);
extern	int		GL_RecvInt(NETFILE *fp ,Bool fNetwork);
extern	void	GL_RecvString(NETFILE *fp, char *str, Bool fNetwork);
extern	Fixed	*GL_RecvFixed(NETFILE *fp, Bool fNetwork);
extern	double	GL_RecvFloat(NETFILE *fp, Bool fNetwork);
extern	Bool	GL_RecvBool(NETFILE *fp, Bool fNetwork);

extern	void	GL_SendValue(NETFILE *fp, ValueStruct *value, char *coding,
							 Bool fBlob, Bool fExpand, Bool fNetwork);
extern	void	GL_RecvValue(NETFILE *fp, ValueStruct *value, char *coding,
							 Bool fBlob, Bool fExpand, Bool fNetwork);

extern	void	InitGL_Comm(void);
#endif
