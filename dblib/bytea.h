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
#define 	MONBLOBEXPIRE 50
#define 	SEQMONBLOB "seqmonblob"

typedef struct _monblob_struct {
	char *id;
	MonObjectType  blobid;
	char importtime[50];
	unsigned int lifetype;
	char *filename;
	int size;
	int status;
	char *content_type;
	char *bytea;
	size_t 	bytea_len;
} monblob_struct;

extern Bool				monblob_setup(DBG_Struct *dbg, Bool recreate);
extern char*			new_id(void);
extern MonObjectType	new_blobid(DBG_Struct *dbg);
extern monblob_struct*	new_monblob_struct(DBG_Struct *dbg,char *id,MonObjectType blobid);
extern void 			free_monblob_struct(monblob_struct *monblob);
extern ValueStruct*		escape_bytea(DBG_Struct *dbg,char *src, size_t len);
extern ValueStruct*		unescape_bytea(DBG_Struct *dbg,ValueStruct *value);
extern Bool 			monblob_insert(DBG_Struct *dbg,monblob_struct *monblob, Bool update);
extern size_t			file_to_bytea(DBG_Struct *dbg,char *filename, ValueStruct **value);
extern char*  			value_to_file(char *filename, ValueStruct *value);

extern char*			monblob_import(DBG_Struct *dbg,char *id, int persist,char *filename,char *content_type, unsigned int lifetype);
extern char*			monblob_import_mem(DBG_Struct *dbg,char *id,int persist,char *filename,char *content_type,unsigned int lifetype,char *buf,size_t size);

extern ValueStruct*		monblob_export(DBG_Struct *dbg, char *id);
extern Bool				monblob_export_file(DBG_Struct *dbg,char *id,char *filename);
extern Bool				monblob_export_mem(DBG_Struct *dbg,char *id,char **buf,size_t *size);
extern Bool 			monblob_persist(DBG_Struct *dbg,char *id,char *filename,char *content_type,unsigned int lifetype);
extern char*			monblob_get_filename(DBG_Struct *dbg,char *id);
extern char*			monblob_get_id(DBG_Struct *dbg, MonObjectType blobid);
extern MonObjectType 	monblob_get_blobid(DBG_Struct *dbg,char *id);
extern Bool 			monblob_delete(DBG_Struct *dbg,char *id);
extern Bool				monblob_check_id(DBG_Struct *dbg,char *id);
extern ValueStruct*		monblob_info(DBG_Struct *dbg, char *id);

extern Bool 			blob_persist(DBG_Struct *dbg,MonObjectType blobid);
extern MonObjectType	blob_import(DBG_Struct *dbg,int persist,char *filename,char *content_type,unsigned int lifetype);
extern MonObjectType	blob_import_mem(DBG_Struct *dbg,int persist,char *filename,char *content_type,unsigned int lifetype,char *buf,size_t size);
extern Bool				blob_export(DBG_Struct *dbg,MonObjectType blobid,char *filename);
extern Bool				blob_export_mem(DBG_Struct *dbg,MonObjectType blobid,char **buf,size_t *size);
extern Bool 			blob_delete(DBG_Struct *dbg,MonObjectType blobid);
extern ValueStruct*		blob_list(DBG_Struct *dbg);
extern Bool				blob_check_id(DBG_Struct *dbg,MonObjectType blobid);
extern ValueStruct*		blob_info(DBG_Struct *dbg, char *blobid);
#endif
