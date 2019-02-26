#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <libgen.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include <libmondai.h>
#include <glib.h>
#include <uuid.h>
#include <json.h>

#include <amqp_tcp_socket.h>
#include <amqp.h>
#include <amqp_framing.h>

#include "directory.h"
#include "dbgroup.h"
#include "dbutils.h"
#include "monsys.h"
#include "option.h"
#include "enum.h"
#include "comm.h"
#include "monpushevent.h"
#include "message.h"
#include "debug.h"

static void reset_id(DBG_Struct *dbg) {
  char *sql;
  ValueStruct *ret;

  sql = "SELECT setval('" SEQMONPUSHEVENT "', 1, false);";
  ret = ExecDBQuery(dbg, sql, FALSE);
  FreeValueStruct(ret);
}

extern int new_monpushevent_id(DBG_Struct *dbg) {
  ValueStruct *ret, *val;
  char *sql;
  int id;

  sql = "SELECT nextval('" SEQMONPUSHEVENT "') AS id;";
  ret = ExecDBQuery(dbg, sql, FALSE);
  if (ret) {
    val = GetItemLongName(ret, "id");
    id = ValueToInteger(val);
    FreeValueStruct(ret);
  } else {
    id = 0;
  }
  if ((unsigned int)id >= USHRT_MAX) {
    reset_id(dbg);
  }
  return id;
}

static Bool create_monpushevent(DBG_Struct *dbg) {
  Bool rc;
  char *sql;

  sql = ""
        "CREATE TABLE " MONPUSHEVENT " ("
        "  uuid      varchar(37) PRIMARY KEY,"
        "  id        int,"
        "  event     varchar(128),"
        "  user_     varchar(128),"
        "  pushed_at timestamp with time zone,"
        "  data	     varchar(2048)"
        ");";
  rc = ExecDBOP(dbg, sql, FALSE);
  if (rc != MCP_OK) {
    Warning("SQL Error:%s", sql);
    return FALSE;
  }
  sql = "CREATE INDEX " MONPUSHEVENT "_uuid ON " MONPUSHEVENT " (uuid);";
  rc = ExecDBOP(dbg, sql, FALSE);
  if (rc != MCP_OK) {
    Warning("SQL Error:%s", sql);
    return FALSE;
  }
  sql = "CREATE INDEX " MONPUSHEVENT "_event ON " MONPUSHEVENT " (event);";
  rc = ExecDBOP(dbg, sql, FALSE);
  if (rc != MCP_OK) {
    Warning("SQL Error:%s", sql);
    return FALSE;
  }
  sql = "CREATE INDEX " MONPUSHEVENT "_user ON " MONPUSHEVENT " (user_);";
  rc = ExecDBOP(dbg, sql, FALSE);
  if (rc != MCP_OK) {
    Warning("SQL Error:%s", sql);
    return FALSE;
  }
  sql = "CREATE INDEX " MONPUSHEVENT "_pushed_at ON " MONPUSHEVENT
        " (pushed_at);";
  rc = ExecDBOP(dbg, sql, FALSE);
  if (rc != MCP_OK) {
    Warning("SQL Error:%s", sql);
    return FALSE;
  }

  sql = "CREATE SEQUENCE " SEQMONPUSHEVENT " ;";
  rc = ExecDBOP(dbg, sql, FALSE);
  if (rc != MCP_OK) {
    Warning("SQL Error:%s", sql);
    return FALSE;
  }

  return TRUE;
}

extern Bool monpushevent_setup(DBG_Struct *dbg) {
  int rc;

  TransactionStart(dbg);
  if (table_exist(dbg, MONPUSHEVENT) != TRUE) {
    create_monpushevent(dbg);
  }
  rc = TransactionEnd(dbg);
  return (rc == MCP_OK);
}

static Bool monpushevent_expire(DBG_Struct *dbg) {
  Bool rc;
  char *sql;

  sql = "DELETE FROM " MONPUSHEVENT
        " WHERE (now() > pushed_at + CAST('" MONPUSHEVENT_EXPIRE
        " days' AS INTERVAL));";
  rc = ExecDBOP(dbg, sql, FALSE);
  return (rc == MCP_OK);
}

static Bool monpushevent_insert(DBG_Struct *dbg, const char *uuid, int id,
                                const char *user, const char *event,
                                const char *pushed_at, const char *data) {
  char *sql, *e_user, *e_event, *e_pushed_at, *e_data;
  int rc;

  monpushevent_expire(dbg);

  e_user = Escape_monsys(dbg, user);
  e_event = Escape_monsys(dbg, event);
  e_pushed_at = Escape_monsys(dbg, pushed_at);
  e_data = Escape_monsys(dbg, data);

  sql = g_strdup_printf("INSERT INTO %s (uuid,id,event,user_,pushed_at,data) "
                        "VALUES ('%s','%d','%s','%s','%s','%s');",
                        MONPUSHEVENT, uuid, id, e_user, e_event, e_pushed_at,
                        e_data);
  rc = ExecDBOP(dbg, sql, FALSE);

  xfree(e_user);
  xfree(e_event);
  xfree(e_pushed_at);
  xfree(e_data);
  g_free(sql);

  return (rc == MCP_OK);
}

json_object *push_event_conv_value(ValueStruct *v) {
  json_object *obj, *child;
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
    return json_object_new_string(ValueToString(v, NULL));
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
      child = push_event_conv_value(ValueArrayItem(v, i));
      if (child != NULL) {
        json_object_array_add(obj, child);
      }
    }
    return obj;
  case GL_TYPE_ROOT_RECORD:
  case GL_TYPE_RECORD:
    obj = json_object_new_object();
    for (i = 0; i < ValueRecordSize(v); i++) {
      child = push_event_conv_value(ValueRecordItem(v, i));
      if (child != NULL) {
        json_object_object_add(obj, ValueRecordName(v, i), child);
      }
    }
    return obj;
  }
  return NULL;
}

static gboolean AMQPSend(const char *event, const char *msg) {
  char *p, *hostname, *exchange, routingkey[SIZE_LONGNAME+1];
  char *user, *pass;
  int port, status, x;
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
  snprintf(routingkey,SIZE_LONGNAME,"tenant.%s.%s", getenv("MCP_TENANT"), event);
  routingkey[SIZE_LONGNAME] = 0;
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
    fprintf(stderr, "creating TCP socket\n");
    goto AMQP_CONN_ERROR;
  }
  status = amqp_socket_open(socket, hostname, port);
  if (status) {
    fprintf(stderr, "opening TCP socket\n");
    goto AMQP_CONN_ERROR;
  }

  reply =
      amqp_login(conn, "/", 0, 131072, 0, AMQP_SASL_METHOD_PLAIN, user, pass);
  if (reply.reply_type != AMQP_RESPONSE_NORMAL) {
    fprintf(stderr, "amqp_login\n");
    goto AMQP_CONN_ERROR;
  }
  amqp_channel_open(conn, 1);
  reply = amqp_get_rpc_reply(conn);
  if (reply.reply_type != AMQP_RESPONSE_NORMAL) {
    fprintf(stderr, "amqp_get_rpc_reply\n");
    goto AMQP_CONN_ERROR;
  }
  {
    if (getenv("PUSHEVENT_LOGGING")) {
      Warning("PushEvent %s %s %s", exchange, routingkey, msg);
    }
    amqp_basic_properties_t props;
    props._flags = AMQP_BASIC_CONTENT_TYPE_FLAG | AMQP_BASIC_DELIVERY_MODE_FLAG;
    props.content_type = amqp_cstring_bytes("application/json");
    props.delivery_mode = 2; /* persistent delivery mode */
    x = amqp_basic_publish(conn, 1, amqp_cstring_bytes(exchange),
                           amqp_cstring_bytes(routingkey), 0, 0, &props,
                           amqp_cstring_bytes(msg));
    if (x < 0) {
      fprintf(stderr, "amqp_basic_publish\n");
      goto AMQP_CONN_ERROR;
    }
  }

  reply = amqp_channel_close(conn, 1, AMQP_REPLY_SUCCESS);
  if (reply.reply_type != AMQP_RESPONSE_NORMAL) {
    fprintf(stderr, "amqp_channel_close\n");
    goto AMQP_CONN_ERROR;
  }

  reply = amqp_connection_close(conn, AMQP_REPLY_SUCCESS);
  if (reply.reply_type != AMQP_RESPONSE_NORMAL) {
    fprintf(stderr, "amqp_connection_close\n");
    goto AMQP_CONN_ERROR;
  }

  x = amqp_destroy_connection(conn);
  if (x < 0) {
    fprintf(stderr, "amqp_destroy_connection\n");
    return FALSE;
  }
  return TRUE;
AMQP_CONN_ERROR:
  fprintf(stderr, "amqp_destroy_connection\n");
  x = amqp_destroy_connection(conn);
  if (x < 0) {
    fprintf(stderr, "amqp_destroy_connection failure\n");
  }
  return FALSE;
}

gboolean push_event_via_value(DBG_Struct *dbg, ValueStruct *val) {
  ValueStruct *body, *event;
  json_object *obj;

  event = GetItemLongName(val,"event");
  if (event == NULL) {
    Warning("no [event] record");
    return FALSE;
  }
  body = GetItemLongName(val,"body");
  if (body == NULL) {
    Warning("no [body] record");
    return FALSE;
  }
  obj = push_event_conv_value(body);
  return push_event_via_json(dbg,ValueToString(event, NULL),obj);
}

gboolean push_event_via_json(DBG_Struct *dbg, const char *event,
                             json_object *body) {
  json_object *obj;
  gboolean ret;
  time_t now;
  struct tm tm_now;
  char uuid[64], str_now[128], *user;
  const char *data;
  int id;
  uuid_t u;

  if (dbg == NULL) {
    Warning("dbg null");
    return FALSE;
  }

  user = getenv("MCP_USER");
  if (user == NULL) {
    Warning("MCP_USER set __nobody");
    user = "__nobody";
  }
  uuid_generate(u);
  uuid_unparse(u, uuid);
  id = new_monpushevent_id(dbg);

  obj = json_object_new_object();
  json_object_object_add(obj, "uuid", json_object_new_string(uuid));
  json_object_object_add(obj, "id", json_object_new_int(id));
  json_object_object_add(obj, "event", json_object_new_string(event));
  json_object_object_add(obj, "user", json_object_new_string(user));
  json_object_object_add(obj, "body", body);
  now = time(NULL);
  localtime_r(&now, &tm_now);
  strftime(str_now, sizeof(str_now), "%FT%T%z", &tm_now);
  json_object_object_add(obj, "time", json_object_new_string(str_now));
  data =
      (const char *)json_object_to_json_string_ext(obj, JSON_C_TO_STRING_PLAIN);

  monpushevent_insert(dbg, uuid, id, event, user, str_now, data);
  ret = AMQPSend(event, data);
  json_object_put(obj);

  return ret;
}
