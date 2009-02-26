/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 2000-2009 Ogochan & JMA (Japan Medical Association).
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

#ifndef	_INC_CONST_H
#define	_INC_CONST_H

#define	DB_LOCALE		"euc-jp"
/*	#define	DB_LOCALE		NULL	*/

#define	PORT_GLTERM			"8000"
#define	PORT_GLAUTH			"8001"
#define	PORT_DBLOG			"8002"
#define	PORT_HTSERV			"8011"
#define	PORT_PGSERV			"8012"
#define	PORT_DBSERV			"8013"

#define	PORT_WFC			"9000"
#define	PORT_WFC_APS		"9001"
#define	PORT_WFC_CONTROL	"9010"
#define	PORT_BLOB			"9011"
#define	PORT_POSTGRES		5432
#define	PORT_REDIRECT		"8010"

#define	PORT_MSGD			"8514"
#define	PORT_FDD			"8515"

#define	CONTROL_PORT	"/tmp/wfc.control:0600"
#define	BLOB_PORT		"/tmp/blob:0600"

#define	SIZE_PASS		(3+8+1+22)
#define	SIZE_OTHER		128

#define	SIZE_BLOCK		1024

#define	SIZE_HOST		255

#define	SIZE_NAME		64
#define	SIZE_EVENT		64
#define	SIZE_TERM		64
#define	SIZE_FUNC		64
#define	SIZE_USER		64
#define	SIZE_RNAME		64
#define	SIZE_PNAME		64
#define	SIZE_STATUS		4
#define	SIZE_PUTTYPE	8
#define	SIZE_STACK		16
#define	SIZE_SESID		64
#define	SIZE_AUTHID		64
#define	SIZE_ARG		255

#ifndef	SIZE_LONGNAME
#define	SIZE_LONGNAME		1024
#endif

#ifndef	SIZE_LARGE_BUFF
#define	SIZE_LARGE_BUFF		1024*1024
#endif

#define	SIZE_DEFAULT_ARRAY_SIZE		64
#define	SIZE_DEFAULT_TEXT_SIZE		256

#endif
