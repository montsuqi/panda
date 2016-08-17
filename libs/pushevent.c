/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 2009 NaCl.
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

/*
#define	DEBUG
#define	TRACE
*/

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include	<errno.h>
#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<stdarg.h>
#include	<string.h>
#include	<ctype.h>
#include	<glib.h>
#include	<signal.h>
#include	<time.h>
#include	<sys/time.h>
#include	<unistd.h>

#include	<amqp_tcp_socket.h>
#include	<amqp.h>
#include	<amqp_framing.h>

#include	"const.h"
#include	"enum.h"
#include	"libmondai.h"
#include	"message.h"
#include	"debug.h"

static	json_object*
MakeBodyJSON(
	ValueStruct *v)
{
	json_object *obj,*child;
	int i;

	if (v == NULL) {
		return NULL;
	}
	switch (v->type) {
	case GL_TYPE_CHAR:
	case GL_TYPE_VARCHAR:
	case GL_TYPE_DBCODE:
	case GL_TYPE_TEXT:
	case GL_TYPE_SYMBOL:
	case GL_TYPE_ALIAS:
	case GL_TYPE_OBJECT:
	case GL_TYPE_BYTE:
	case GL_TYPE_BINARY:
	case GL_TYPE_TIMESTAMP:
	case GL_TYPE_DATE:
	case GL_TYPE_TIME:
		return json_object_new_string(ValueToString(v,NULL));
	case GL_TYPE_BOOL:
		return json_object_new_boolean(ValueBool(v));
	case GL_TYPE_INT:
		return json_object_new_int(ValueInteger(v));
	case GL_TYPE_NUMBER:
	case GL_TYPE_FLOAT:
		return json_object_new_double(ValueToFloat(v));
	case GL_TYPE_ARRAY:
		obj = json_object_new_array();
		for (i = 0; i < ValueArraySize(v); i++) {
			child = MakeBodyJSON(ValueArrayItem(v,i));
			if (child != NULL) {
				json_object_array_add(obj,child);
			}
		}
		return obj;
	case GL_TYPE_RECORD:
		obj = json_object_new_object();
		for	(i = 0; i < ValueRecordSize(v); i++) {
			child = MakeBodyJSON(ValueRecordItem(v,i));
			if (child != NULL) {
				json_object_object_add(obj,ValueRecordName(v,i),child);
			}
		}
		return obj;
	}
	return NULL;
}

static	json_object*
MakeJSON(
	const char *event,
	ValueStruct *v)
{
	json_object *obj,*body;
	time_t now;
	struct tm tm_now;
	char str_now[128],*user;

	obj = json_object_new_object();
	json_object_object_add(obj,"event",json_object_new_string(event));
	user = getenv("MCP_USER");
	if (user == NULL) {
		Warning("MCP_USER set __nobody");
		user = "__nobody";
	}
	json_object_object_add(obj,"user",json_object_new_string(user));

	body = MakeBodyJSON(v);
	json_object_object_add(obj,"body",body);

	now = time(NULL);
	localtime_r(&now, &tm_now);
	strftime(str_now,sizeof(str_now),"%FT%T%z",&tm_now);
	json_object_object_add(obj,"time",json_object_new_string(str_now));
	return obj;
}

static	gboolean
AMQPSend(
	const char* event,
	const char* msg)
{
	char *p,*hostname,*exchange,*routingkey;
	char *user,*pass;
	int port, status,x;
	amqp_socket_t *socket = NULL;
	amqp_connection_state_t conn;
	amqp_rpc_reply_t reply;

	hostname = "localhost";
	if ((p = getenv("MON_AMQP_SERVER_HOST")) != NULL) {
		hostname = p;
	}
	port = 5672;
	if ((p = getenv("MON_AMQP_SERVER_PORT")) != NULL) {
		port = atoi(p);
	}
	exchange = "amq.topic";
	if ((p = getenv("MON_AMQP_EXCHANGE")) != NULL) {
		exchange = p;
	}
	routingkey = g_strdup_printf("tenant.%s.%s",getenv("MCP_TENANT"),event);
	user = "guest";
	if ((p = getenv("MON_AMQP_USER")) != NULL) {
		user = p;
	}
	pass = "guest";
	if ((p = getenv("MON_AMQP_PASS")) != NULL) {
		pass = p;
	}

	conn = amqp_new_connection();
	socket = amqp_tcp_socket_new(conn);
	if (!socket) {
		fprintf(stderr,"creating TCP socket");
		return FALSE;
	}
	status = amqp_socket_open(socket, hostname, port);
	if (status) {
		fprintf(stderr,"opening TCP socket");
		return FALSE;
	}

	reply = amqp_login(conn,"/",0,131072,0,AMQP_SASL_METHOD_PLAIN,user,pass);
	if (reply.reply_type != AMQP_RESPONSE_NORMAL) {
		fprintf(stderr,"amqp_login");
		return FALSE;
	}
	amqp_channel_open(conn, 1);
	reply = amqp_get_rpc_reply(conn);
	if (reply.reply_type != AMQP_RESPONSE_NORMAL) {
		fprintf(stderr,"amqp_get_rpc_reply");
		return FALSE;
	}
	{
		if (getenv("PUSHEVENT_LOGGING")) {
			Warning("PushEvent %s %s %s",exchange,routingkey,msg);
		}
		amqp_basic_properties_t props;
		props._flags = AMQP_BASIC_CONTENT_TYPE_FLAG | AMQP_BASIC_DELIVERY_MODE_FLAG;
		props.content_type = amqp_cstring_bytes("application/json");
		props.delivery_mode = 2; /* persistent delivery mode */
		x = amqp_basic_publish(conn,
				1,
				amqp_cstring_bytes(exchange),
				amqp_cstring_bytes(routingkey),
				0,
				0,
				&props,
				amqp_cstring_bytes(msg));
		if (x < 0) {
			fprintf(stderr,"amqp_basic_publish");
			return FALSE;
		}
	}
	g_free(routingkey);
	
	reply = amqp_channel_close(conn, 1, AMQP_REPLY_SUCCESS);
	if (reply.reply_type != AMQP_RESPONSE_NORMAL) {
		fprintf(stderr,"amqp_channel_close");
		return FALSE;
	}

	reply = amqp_connection_close(conn, AMQP_REPLY_SUCCESS);
	if (reply.reply_type != AMQP_RESPONSE_NORMAL) {
		fprintf(stderr,"amqp_connection_close");
		return FALSE;
	}

	x = amqp_destroy_connection(conn);
	if (x < 0) {
		fprintf(stderr,"amqp_destroy_connection");
		return FALSE;
	}
	return TRUE;
}

gboolean
PushEvent(
	ValueStruct		*val)
{
	ValueStruct *v,*body;
	json_object *obj;
	char        *event;
	gboolean    ret;
ENTER_FUNC;
	if ((v = GetItemLongName(val,"event"))  ==  NULL) {
		Warning("invalid argument value: need 'event'");
		return FALSE;
	}
	event = ValueToString(v,NULL);
	if ((body = GetItemLongName(val,"body"))  ==  NULL) {
		Warning("invalid argument value: need 'body'");
		return FALSE;
	}
	obj = MakeJSON(event,body);
	ret = AMQPSend(event,(const char*)json_object_to_json_string(obj));
	json_object_put(obj);
LEAVE_FUNC;
	return ret;
}
