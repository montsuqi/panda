/*	PANDA -- a simple transaction monitor

Copyright (C) 2004 Ogochan

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

#ifndef	_INC_CGI_H
#define	_INC_CGI_H

#define	CGIV_NULL		0x00
#define	CGIV_SAVE		0x01

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

GLOBAL	GHashTable	*Values;
GLOBAL	GHashTable	*Files;
GLOBAL	char		*Codeset;
GLOBAL	Bool		fGet;
GLOBAL	Bool		fCookie;
GLOBAL	Bool		fDump;
GLOBAL	Bool		fDebug;
GLOBAL	Bool		fJavaScriptLink;
GLOBAL	char		*SesDir;
GLOBAL	char		*CommandLine;
GLOBAL	time_t		SesExpire;
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
