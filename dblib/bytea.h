/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 2001-2009 Ogochan & JMA (Japan Medical Association).
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

#ifndef	_BYTEA_H
#define	_BYTEA_H

#include	"dblib.h"
#define 	MONBLOB	"monblob"
#define 	BLOBEXPIRE 2

typedef struct _monblob_struct {
	char *id;
	int blobid;
	char importtime[50];
	unsigned int lifetype;
	char *filename;
	int size;
	char *content_type;
	char *bytea;
	size_t 	bytea_len;
} monblob_struct;

extern Bool monblob_setup(DBG_Struct *dbg);
extern char *new_id(void);
extern monblob_struct *NewMonblob_struct(char *id);
extern void FreeMonblob_struct(monblob_struct *monblob);
extern ValueStruct *escape_bytea(DBG_Struct *dbg, unsigned char *src, size_t len);
extern ValueStruct *unescape_bytea(DBG_Struct *dbg, ValueStruct *value);
extern Bool monblob_insert(DBG_Struct	*dbg, monblob_struct *monblob, Bool update);
extern int file_to_bytea(DBG_Struct *dbg, char *filename, ValueStruct **value);
extern	char *monblob_import(DBG_Struct *dbg, char *id, char *filename, char *content_type, unsigned int lifetype);
extern char *value_to_file(char *filename, ValueStruct *value);
extern char *monblob_export(DBG_Struct *dbg, char *id, char *filename);
extern	char *monblob_getid(DBG_Struct *dbg, int blobid);
extern	void monblob_delete(DBG_Struct	*dbg, char *id);

#endif
