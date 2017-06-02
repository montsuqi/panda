/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 1998-1999 Ogochan.
 * Copyright (C) 2000-2008 Ogochan & JMA (Japan Medical Association).
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

#ifndef	_INC_PROTOCOL_H
#define	_INC_PROTOCOL_H

#include	<json.h>
#include	<curl/curl.h>
#ifdef USE_SSL
#include	<openssl/engine.h>
#endif
#include	<libmondai.h>

typedef struct {
	CURL			*Curl;
	char			*AuthURI;
	char			*RPCURI;
	char			*RESTURI;
	int	            RPCID;
	char			*SessionID;
	char			*ProtocolVersion;
	char			*AppVersion;
	gboolean		fGinbee;
#ifdef USE_SSL
	gboolean		fSSL;
	gboolean		fPKCS11;
	ENGINE			*Engine;
#endif
} GLPctx;

void 				RPC_GetServerInfo(GLPctx *);
void 				RPC_StartSession(GLPctx *);
json_object* 		RPC_GetScreenDefine(GLPctx *,const char*);
void 				RPC_EndSession(GLPctx *);
json_object* 		RPC_GetWindow(GLPctx *);
json_object*		RPC_SendEvent(GLPctx *,json_object *params);
void				RPC_GetMessage(GLPctx * ,char**d,char**p,char**a);
json_object*		RPC_ListDownloads(GLPctx *);
char*				REST_PostBLOB(GLPctx *,LargeByteString *);
LargeByteString*	REST_GetBLOB(GLPctx *,const char *);
void				GLP_SetRPCURI(GLPctx *,const char *);
void				GLP_SetRESTURI(GLPctx *,const char *);
void				GLP_SetSessionID(GLPctx *,const char *);
void				GLP_SetRPCID(GLPctx *,int);
char*				GLP_GetRPCURI(GLPctx *);
char*				GLP_GetRESTURI(GLPctx *);
char*				GLP_GetSessionID(GLPctx *);
int					GLP_GetRPCID(GLPctx *);
GLPctx*				InitProtocol(const char *auth_uri,const char *user,const char *pass);
#ifdef USE_SSL
void				GLP_SetSSL(GLPctx*,const char*cert,const char*key,const char*pass,const char*cafile);
void				GLP_SetSSLPKCS11(GLPctx *,const char *p11lib,const char *pin);
#endif
void				FinalProtocol(GLPctx *);

#endif
