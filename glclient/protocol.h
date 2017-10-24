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
	char			*PusherURI;
	int	            RPCID;
	char			*SessionID;
	char			*ProtocolVersion;
	char			*AppVersion;
	gboolean		fGinbee;
	size_t			ReqSize;
	size_t			ResSize;
	unsigned long	RPCExecTime;
	unsigned long	TotalExecTime;
	unsigned long	AppExecTime;
#ifdef USE_SSL
	gboolean		fSSL;
	gboolean		fPKCS11;
	ENGINE			*Engine;
#endif
} GLProtocol;

void 				RPC_GetServerInfo(GLProtocol *);
void 				RPC_StartSession(GLProtocol *);
json_object* 		RPC_GetScreenDefine(GLProtocol *,const char*);
void 				RPC_EndSession(GLProtocol *);
json_object* 		RPC_GetWindow(GLProtocol *);
json_object*		RPC_SendEvent(GLProtocol *,json_object *params);
void				RPC_GetMessage(GLProtocol * ,char**d,char**p,char**a);
json_object*		RPC_ListDownloads(GLProtocol *);
char*				REST_PostBLOB(GLProtocol *,LargeByteString *);
LargeByteString*	REST_GetBLOB(GLProtocol *,const char *);
LargeByteString*	REST_GetBLOB_via_ENV();

gboolean			GLP_GetfGinbee(GLProtocol*);
void				GLP_SetRPCURI(GLProtocol *,const char *);
char*				GLP_GetRPCURI(GLProtocol *);
void				GLP_SetRESTURI(GLProtocol *,const char *);
char*				GLP_GetRESTURI(GLProtocol *);
void				GLP_SetPusherURI(GLProtocol *,const char *);
char*				GLP_GetPusherURI(GLProtocol *);
void				GLP_SetSessionID(GLProtocol *,const char *);
char*				GLP_GetSessionID(GLProtocol *);
void				GLP_SetRPCID(GLProtocol *,int);
int					GLP_GetRPCID(GLProtocol *);


GLProtocol*			InitProtocol(const char *auth_uri,const char *user,const char *pass);
#ifdef USE_SSL
void				GLP_SetSSL(GLProtocol*,const char*cert,const char*key,const char*pass,const char*cafile);
void				GLP_SetSSLPKCS11(GLProtocol *,const char *p11lib,const char *pin);
#endif
void				FinalProtocol(GLProtocol *);

#endif
