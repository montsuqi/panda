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

#ifndef	_INC_DI_PARSER_H
#define	_INC_DI_PARSER_H
#include	<glib.h>

#define	MULTI_NO		0
#define	MULTI_DB		1
#define	MULTI_LD		2
#define	MULTI_ID		3
#define	MULTI_APS		4

#include	"LDparser.h"
#include	"BDparser.h"
#include	"DBDparser.h"
#include	"dbgroup.h"

typedef	struct {
	char		*name;
	char		*BaseDir
	,			*LD_Dir
	,			*BD_Dir
	,			*DBD_Dir
	,			*RecordDir;
	Port		*WfcApsPort;
	size_t		cLD
	,			cBD
	,			cDBD
	,			linksize
	,			stacksize;
	RecordStruct	*mcprec;
	RecordStruct	*linkrec;
	GHashTable	*LD_Table;
	GHashTable	*BD_Table;
	GHashTable	*DBD_Table;
	LD_Struct	**ld;
	BD_Struct	**bd;
	DBD_Struct	**db;
	int			mlevel;
	int			cDBG;
	DBG_Struct	**DBG;
	GHashTable	*DBG_Table;
}	DI_Struct;

#undef	GLOBAL
#ifdef	_DI_PARSER
#define	GLOBAL		/*	*/
#else
#define	GLOBAL		extern
#endif


#undef	GLOBAL

extern	void		DI_ParserInit(void);
extern	DI_Struct	*DI_Parser(char *name, char *ld, char *bd, char *db);
#endif
