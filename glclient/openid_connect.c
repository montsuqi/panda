#include <glib.h>
#include <json.h>
#include "openid_connect.h"
#include "logger.h"
#include "libmondai.h"
#include "gettext.h"

#define OPENID_HTTP_GET 1
#define OPENID_HTTP_POST 2

CURL *OpenIdConnectInitCURL() {
  CURL *Curl;

  Curl = curl_easy_init();
  if (!Curl) {
    Warning("curl_easy_init failure");
    exit(0);
  }

  return Curl;
}

size_t open_id_write_data(void *buf, size_t size, size_t nmemb, void *userp) {
  size_t buf_size;
  LargeByteString *lbs;
  unsigned char *p;
  int i;

  lbs = (LargeByteString *)userp;
  buf_size = size * nmemb;
  for (i = 0, p = (unsigned char *)buf; i < buf_size; i++, p++) {
    LBS_EmitChar(lbs, *p);
  }
  return buf_size;
}

json_object *parse_header_text(char *header_text) {
  json_object *result;
  result = json_object_new_object();

  char *ptr, *value;
  ptr = strtok(header_text, "\n");
  while (ptr != NULL) {
    value = strchr(ptr, ':');
    if (value) {
      *value = '\0';
      value += sizeof(char);
      json_object_object_add(result, ptr, json_object_new_string(value));
    }
    ptr = strtok(NULL, "\n");
  }
  return result;
}

json_object *request(CURL *curl, char *uri, int method, json_object *params) {
  LargeByteString *body, *headers;
  json_object *res, *res_headers;

  body = NewLBS();
  headers = NewLBS();

  curl_easy_setopt(curl, CURLOPT_URL, uri);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)body);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, open_id_write_data);
  curl_easy_setopt(curl, CURLOPT_HEADERDATA, (void *)headers);
  curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, open_id_write_data);

  curl_easy_perform(curl);

  res = json_tokener_parse(LBS_Body(body));
  res_headers = parse_header_text(LBS_Body(headers));
  json_object_object_add(res, "headers", res_headers);

  curl_easy_cleanup(curl);
  
  return res;
}

void doAuthenticationRequestToRP(OpenIdConnectProtocol *oip) {
  json_object *obj, *result, *headers;
  obj = json_object_new_object();

  result = request(oip->Curl, oip->SP_URI, OPENID_HTTP_GET, obj);

  if (!json_object_object_get_ex(result, "client_id", &obj)) {
    Error(_("no client_id object"));
  }
  oip->ClientId = g_strdup(json_object_get_string(obj));

  if (!json_object_object_get_ex(result, "state", &obj)) {
    Error(_("no state object"));
  }
  oip->State = g_strdup(json_object_get_string(obj));

  if (!json_object_object_get_ex(result, "redirect_uri", &obj)) {
    Error(_("no redirect_uri object"));
  }
  oip->RedirectURI = g_strdup(json_object_get_string(obj));

  if (!json_object_object_get_ex(result, "nonce", &obj)) {
    Error(_("no nonce object"));
  }
  oip->Nonce = g_strdup(json_object_get_string(obj));

  json_object_object_get_ex(result, "headers", &headers);

  if (!json_object_object_get_ex(headers, "Set-Cookie", &obj)) {
    Error(_("no Set-Cookie object"));
  }
  oip->RPCookie = g_strdup(json_object_get_string(obj));

  if (!json_object_object_get_ex(headers, "Location", &obj)) {
    Error(_("no Location object"));
  }
  oip->AuthenticationRequestURI = g_strdup(json_object_get_string(obj));
}

void doAuthenticationRequestToIp(OpenIdConnectProtocol *oip) {
}

void doLoginToIP(OpenIdConnectProtocol *oip) {
}

void doLoginToRP(OpenIdConnectProtocol *oip) {
}

extern OpenIdConnectProtocol *InitOpenIdConnectProtocol(const char *sso_sp_uri, const char *user, const char * pass) {
  OpenIdConnectProtocol *oip;
  oip = g_new0(OpenIdConnectProtocol, 1);

  oip->User = g_strdup(user);
  oip->Password = g_strdup(pass);
  oip->SP_URI = g_strdup(sso_sp_uri);

  oip->Curl = OpenIdConnectInitCURL();

  return oip;
}

extern char *StartOpenIdConnect(OpenIdConnectProtocol *oip) {
  // バックエンドサーバへのログイン要求
  doAuthenticationRequestToRP(oip);
  // 認証サーバへのログイン要求
  doAuthenticationRequestToIp(oip);
  // 認証サーバへのログイン
   doLoginToIP(oip);
  // バックエンドサーバへの session_id 発行要求
  doLoginToRP(oip);

  return oip->RPCookie;
}
