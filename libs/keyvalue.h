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

#ifndef	_INC_KEYVALUE_H
#define	_INC_KEYVALUE_H

typedef	struct _KV_State	{
	GHashTable	*table;
	pthread_mutex_t	mutex;
}	KV_State;

extern	KV_State	*InitKV(void);
extern	void		FinishKV(KV_State *satte);

extern	int		KV_GetValue(KV_State *state, ValueStruct *arg);
extern	int		KV_SetValue(KV_State *state, ValueStruct *arg);
extern	int		KV_SetValueALL(KV_State *state, ValueStruct *arg);
extern	int		KV_ListKey(KV_State *state, ValueStruct *arg);
extern	int		KV_ListEntry(KV_State *state, ValueStruct *arg);
extern	int		KV_NewEntry(KV_State *state, ValueStruct *arg);
extern	int		KV_DeleteEntry(KV_State *state, ValueStruct *arg);

#undef	GLOBAL
#ifdef	MAIN
#define	GLOBAL		/*	*/
#else
#define	GLOBAL		extern
#endif

#endif
