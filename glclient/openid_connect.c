#include <glib.h>
#include <json.h>
#include <ctype.h>
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

char *trim(char *str) {
  int i;
  if (str == NULL) {
    return str;
  }
  i = strlen(str);
  while (--i > 0 && isspace(str[i]));
  str[i+1] = '\0';
  while (isspace(*str)) str++;
  return str;
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
      value = trim(value);
      json_object_object_add(result, ptr, json_object_new_string(value));
    }
    ptr = strtok(NULL, "\n");
  }
  return result;
}

json_object *request(CURL *curl, char *uri, int method, json_object *params, char *cookie) {
  LargeByteString *body, *headers;
  json_object *res, *res_headers;
  struct curl_slist *request_headers = NULL;
  int response_code;

  body = NewLBS();
  headers = NewLBS();

  curl_easy_setopt(curl, CURLOPT_URL, uri);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)body);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, open_id_write_data);
  curl_easy_setopt(curl, CURLOPT_HEADERDATA, (void *)headers);
  curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, open_id_write_data);
  if (cookie) {
    char buf[1024];
    snprintf(buf, sizeof(buf), "Cookie: %s", cookie);
    request_headers = curl_slist_append(request_headers, buf);
  }
  request_headers = curl_slist_append(request_headers, "Accept: application/json");
  if (method == OPENID_HTTP_POST) {
    request_headers = curl_slist_append(request_headers, "Content-Type: application/json");
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_object_to_json_string_ext(params, JSON_C_TO_STRING_PLAIN));
  }
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, request_headers);

  curl_easy_perform(curl);

  res = json_tokener_parse(LBS_Body(body));
  if (res == NULL) {
    res = json_object_new_object();
  }
  res_headers = parse_header_text(LBS_Body(headers));
  json_object_object_add(res, "headers", res_headers);
  curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
  json_object_object_add(res, "response_code", json_object_new_int(response_code));

  json_object_put(params);
  curl_slist_free_all(request_headers);
  curl_easy_reset(curl);

  return res;
}

void doAuthenticationRequestToRP(OpenIdConnectProtocol *oip) {
  json_object *obj, *result, *headers;
  obj = json_object_new_object();

  result = request(oip->Curl, oip->SP_URI, OPENID_HTTP_GET, obj, NULL);

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

  json_object_put(result);
}

void doAuthenticationRequestToIp(OpenIdConnectProtocol *oip) {
  json_object *params, *result, *obj;
  params = json_object_new_object();
  json_object_object_add(params, "response_type", json_object_new_string("code"));
  json_object_object_add(params, "scope", json_object_new_string("openid"));
  json_object_object_add(params, "client_id", json_object_new_string(oip->ClientId));
  json_object_object_add(params, "state", json_object_new_string(oip->State));
  json_object_object_add(params, "redirect_uri", json_object_new_string(oip->RedirectURI));
  json_object_object_add(params, "nonce", json_object_new_string(oip->Nonce));

  result = request(oip->Curl, oip->AuthenticationRequestURI, OPENID_HTTP_POST, params, NULL);

  if (!json_object_object_get_ex(result, "request_url", &obj)) {
    Error(_("no request_url object"));
  }
  oip->RequestURL = g_strdup(json_object_get_string(obj));

  json_object_put(result);
}

void doLoginToIP(OpenIdConnectProtocol *oip) {
  json_object *params, *result, *obj, *headers;
  params = json_object_new_object();
  json_object_object_add(params, "response_type", json_object_new_string("code"));
  json_object_object_add(params, "scope", json_object_new_string("openid"));
  json_object_object_add(params, "client_id", json_object_new_string(oip->ClientId));
  json_object_object_add(params, "state", json_object_new_string(oip->State));
  json_object_object_add(params, "redirect_uri", json_object_new_string(oip->RedirectURI));
  json_object_object_add(params, "nonce", json_object_new_string(oip->Nonce));
  json_object_object_add(params, "login_id", json_object_new_string(oip->User));
  json_object_object_add(params, "password", json_object_new_string(oip->Password));

  result = request(oip->Curl, oip->RequestURL, OPENID_HTTP_POST, params, NULL);

  json_object_object_get_ex(result, "response_code", &obj);
  if (json_object_get_int(obj) == 403) {
    Error(_("User ID or Password Incorrect"));
    exit(0);
  }

  json_object_object_get_ex(result, "headers", &headers);

  if (!json_object_object_get_ex(headers, "Location", &obj)) {
    Error(_("no Location object"));
  }
  oip->GetSessionURI = g_strdup(json_object_get_string(obj));

  json_object_put(result);
}

void doLoginToRP(OpenIdConnectProtocol *oip) {
  json_object *obj, *result;
  obj = json_object_new_object();

  result = request(oip->Curl, oip->GetSessionURI, OPENID_HTTP_GET, obj, oip->RPCookie);

  if (!json_object_object_get_ex(result, "session_id", &obj)) {
    Error(_("no session_id object"));
  }
  oip->SessionID = g_strdup(json_object_get_string(obj));

  json_object_put(result);
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

extern void StartOpenIdConnect(OpenIdConnectProtocol *oip) {
  // バックエンドサーバへのログイン要求
  doAuthenticationRequestToRP(oip);
  // 認証サーバへのログイン要求
  doAuthenticationRequestToIp(oip);
  // 認証サーバへのログイン
  doLoginToIP(oip);
  // バックエンドサーバへの session_id 発行要求
  doLoginToRP(oip);
}
