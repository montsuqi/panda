/*	PANDA -- a simple transaction monitor

Copyright (C) 2002-2003 Ogochan & JMA (Japan Medical Association).

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
#ifndef	_INC_HTC_H
#define	_INC_HTC_H

#include	"libmondai.h"

typedef	struct {
	LargeByteString	*code;
	GHashTable	*Trans;
	GHashTable	*Radio;
	GHashTable	*FileSelection;
    char *DefaultEvent;
    size_t EnctypePos;
    int FormNo;
}	HTCInfo;

typedef	struct {
    char *filename;
    char *filesize;
}	FileSelectionInfo;

typedef	ValueStruct	*(*GET_VALUE)(char *name, Bool fClear);

extern	char	*GetHostValue(char *name, Bool fClear);
extern	void	InitHTC(char *script_name, GET_VALUE func);

#include	"tags.h"
#include	"HTCparser.h"
#include	"exec.h"

#endif
