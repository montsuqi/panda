/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 2009 JMA (Japan Medical Association).
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

#ifndef _HTTP_H
#define _HTTP_H
#include "port.h"
#include "net.h"

#undef GLOBAL
#ifdef MAIN
#define GLOBAL /*	*/
#else
#define GLOBAL extern
#endif

#define HTTP_CONTINUE 100
#define HTTP_SWITCHING_PROTOCOLS 101
#define HTTP_PROCESSING 102

#define HTTP_OK 200
#define HTTP_CREATED 201
#define HTTP_ACCEPTED 202
#define HTTP_NON_AUTHORITATIVE_INFORMATION 203
#define HTTP_NO_CONTENT 204
#define HTTP_RESET_CONTENT 205
#define HTTP_PARTIAL_CONTENT 206
#define HTTP_MULTI_STATUS 207

#define HTTP_MULTIPLE_CHOICES 300
#define HTTP_MOVED_PERMANENTLY 301
#define HTTP_FOUND 302
#define HTTP_SEE_OTHER 303
#define HTTP_NOT_MODIFIED 304
#define HTTP_USE_PROXY 305
#define HTTP_SWITCH_PROXY 306
#define HTTP_TEMPORARY_REDIRECT 307

#define HTTP_BAD_REQUEST 400
#define HTTP_UNAUTHORIZED 401
#define HTTP_PAYMENT_REQUIRED 402
#define HTTP_FORBIDDEN 403
#define HTTP_NOT_FOUND 404
#define HTTP_METHOD_NOT_ALLOWED 405
#define HTTP_METHOD_NOT_ACCEPTABLE 406
#define HTTP_PROXY_AUTHENTICATION_REQUIRED 407
#define HTTP_REQUEST_TIMEOUT 408
#define HTTP_CONFLICT 409
#define HTTP_GONE 410
#define HTTP_LENGTH_REQUIRED 411
#define HTTP_PRECONDITION_FAILED 412
#define HTTP_REQUEST_ENTITY_TOO_LARGE 413
#define HTTP_REQUEST_URI_TOO_LONG 414
#define HTTP_UNSUPPORTED_MEDIA_TYPE 415
#define HTTP_REQUESTED_RANGE_NOT_SATISFIABLE 416
#define HTTP_EXPECTATION_FAILED 417
#define HTTP_UNPROCESSABLE_ENTITY 422
#define HTTP_LOCKED 423
#define HTTP_FAILED_DEPENDENCY 424
#define HTTP_UNORDERED_COLLECTION 425
#define HTTP_UPGRADE_REQUIRED 426
#define HTTP_RETRY_WITH 449

#define HTTP_INTERNAL_SERVER_ERROR 500
#define HTTP_NOT_IMPLEMENTED 501
#define HTTP_BAD_GATEWAY 502
#define HTTP_SERVICE_UNAVAILABLE 503
#define HTTP_GATEWAY_TIMEOUT 504
#define HTTP_HTTP_VERSION_NOT_SUPPORTED 505
#define HTTP_VARIANT_ALSO_NEGOTIATES 506
#define HTTP_INSUFFICIENT_STORAGE 507
#define HTTP_BANDWIDTH_LIMIT_EXCEEDED 509
#define HTTP_NOT_EXTENDED 510

typedef struct {
  NETFILE *fp;
  int type;
  char host[SIZE_HOST + 1];
  char *agent;
  char *server_host;
  PacketClass method;
  size_t buf_size;
  char *buf;
  char *head;
  char *arguments;
  int body_size;
  char *body;
  GHashTable *header_hash;
  char *user;
  char *pass;
  char *ld;
  char *window;
  char *session_id;
  char *oid;
  gboolean require_auth;
  int status;
} HTTP_REQUEST;

typedef struct {
  int body_size;
  char *body;
  char *content_type;
  char *filename;
  int status;
} HTTP_RESPONSE;

void HTTP_Method(NETFILE *fpComm);

#endif
