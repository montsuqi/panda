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
#define	PORT_POSTGRES		5432
#define	PORT_REDIRECT		"8010"

#define	PORT_MSGD			"8514"
#define	PORT_FDD			"8515"

#define	CONTROL_PORT	"/tmp/wfc.control:0600"

#define	SIZE_PASS		3+8+22
#define	SIZE_OTHER		128

#define	SIZE_BLOCK		1024

#define	SIZE_HOST		255

#define	SIZE_NAME		64
#define	SIZE_EVENT		64
#define	SIZE_TERM		64
#define	SIZE_FUNC		16
#define	SIZE_USER		16
#define	SIZE_RNAME		16
#define	SIZE_PNAME		16
#define	SIZE_STATUS		4
#define	SIZE_PUTTYPE	8
#define	SIZE_STACK		16
#define	SIZE_TRID		16
#define	SIZE_SESID		16
#define	SIZE_ARG		255
#define	SIZE_FUNC		16

#ifndef	SIZE_LONGNAME
#define	SIZE_LONGNAME		1024
#endif

#define	SIZE_DEFAULT_ARRAY_SIZE		64
#define	SIZE_DEFAULT_TEXT_SIZE		256

#endif
