/*
PANDA -- a simple transaction monitor
Copyright (C) 2004-2005 Ogochan.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
*/

#ifndef	_INC_CGI_H
#define	_INC_CGI_H

#include	"LBSfunc.h"

#define	CGIV_NULL		0x00
#define	CGIV_SAVE		0x01

#define	AGENT_TYPE	word
#define	AGENT_TYPE_NULL		(AGENT_TYPE)0x0000
#define	AGENT_TYPE_MSIE		(AGENT_TYPE)0x8000
#define	AGENT_TYPE_MSIE_OLD	(AGENT_TYPE)0x8100
#define	AGENT_TYPE_GECKO	(AGENT_TYPE)0x4000
#define	AGENT_TYPE_MOZILLA	(AGENT_TYPE)0x4100
#define	AGENT_TYPE_FF		(AGENT_TYPE)0x4200
#define	AGENT_TYPE_NS6		(AGENT_TYPE)0x4400
#define	AGENT_TYPE_OPERA	(AGENT_TYPE)0x2000
#define	AGENT_TYPE_NN		(AGENT_TYPE)0x1010
#define	AGENT_TYPE_TEXT		(AGENT_TYPE)0x0800

#define	AGENT_TYPE_JS		(AGENT_TYPE)0x0001
#define	AGENT_TYPE_OLD		(AGENT_TYPE)0x0002

typedef	struct {
	char	*name;
	char	*body;
	char	*(*inFilter)(char *in);
	char	*(*outFilter)(char *out);
	Bool	fSave;
}	CGIValue;

#undef	GLOBAL
#ifdef	MAIN
#define	GLOBAL		/*	*/
#else
#define	GLOBAL		extern
#endif

GLOBAL	AGENT_TYPE	AgentType;
GLOBAL	GHashTable	*Values;
GLOBAL	GHashTable	*Files;
GLOBAL	char		*Codeset;
GLOBAL	Bool		fGet;
GLOBAL	Bool		fCookie;
GLOBAL	Bool		fDump;
GLOBAL	Bool		fDebug;
GLOBAL	Bool		fJavaScriptLink;
GLOBAL	Bool		fJavaScript;
GLOBAL	char		*SesDir;
GLOBAL	char		*CommandLine;
GLOBAL	time_t		SesExpire;
GLOBAL	char		*ScreenDir;
#undef	GLOBAL

extern  void	CGI_InitValues(void);
extern	void	InitCGI(void);

extern	char	*ConvUTF8(unsigned char *istr, char *code);
extern	char	*ConvLocal(char *istr);

extern	void	LBS_EmitUTF8(LargeByteString *lbs, char *str, char *codeset);
extern	void	GetArgs(void);
extern	void	PutHTML(LargeByteString *html);
extern	void	DumpValues(LargeByteString *html, GHashTable *args);
extern	void	Dump(void);
extern	char	*SaveValue(char *name, char *value, Bool fSave);
extern	char	*SaveArgValue(char *name, char *value, Bool fSave);
extern	char	*LoadValue(char *name);
extern	void	RemoveValue(char *name);
extern	void	SetSave(char *name, Bool fSave);
extern  void	SetFilter(char *name, char *(*inFilter)(char *in),
						  char *(*outFilter)(char *out));

extern  void	CheckSessionExpire(void);
extern	Bool	GetSessionValues(void);
extern	void	DeleteSessionValues(void);
extern	Bool	PutSessionValues(void);
extern	void	ClearValues(void);
extern	void	WriteLargeString(FILE *output, LargeByteString *lbs, char *codeset);

#endif
