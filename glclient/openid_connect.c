#include <glib.h>
#include <json.h>
#include <ctype.h>


#include "glclient.h"
#include "gettext.h"
#include "openid_connect.h"
#include "logger.h"
#include "libmondai.h"

#define OPENID_HTTP_GET 1
#define OPENID_HTTP_POST 2
#define PANDA_CLIENT_VERSION "2.0.1"

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

json_object *request(OpenIdConnectProtocol *oip, char *uri, int method, json_object *params, char *cookie) {
  CURL *curl = oip->Curl;
  LargeByteString *body, *headers;
  json_object *res = NULL, *res_headers;
  struct curl_slist *request_headers = NULL;
  int response_code;

  body = NewLBS();
  headers = NewLBS();

  if (getenv("GLCLIENT_CURL_DEBUG") != NULL) {
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);
  }
  if (oip->fSSL) {
    curl_easy_setopt(curl, CURLOPT_USE_SSL, CURLUSESSL_ALL);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2);
    if (strlen(oip->CertFile) > 0 && strlen(oip->CertKeyFile) > 0) {
      curl_easy_setopt(curl, CURLOPT_SSLCERT, oip->CertFile);
      curl_easy_setopt(curl, CURLOPT_SSLCERTTYPE, "PEM");
      curl_easy_setopt(curl, CURLOPT_SSLKEY, oip->CertKeyFile);
      curl_easy_setopt(curl, CURLOPT_SSLKEYTYPE, "PEM");
      curl_easy_setopt(curl, CURLOPT_SSLKEYPASSWD, oip->CertPass);
    }
    curl_easy_setopt(curl, CURLOPT_CAINFO, oip->CAFile);
  }
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
  request_headers = curl_slist_append(request_headers, "X-Support-SSO: 1");
  if (method == OPENID_HTTP_POST) {
    request_headers = curl_slist_append(request_headers, "Content-Type: application/json");
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_object_to_json_string_ext(params, JSON_C_TO_STRING_PLAIN));
  }
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, request_headers);

  curl_easy_perform(curl);
  LBS_EmitEnd(body);
  LBS_EmitEnd(headers);

  res = json_tokener_parse(LBS_Body(body));
  if (res == NULL || is_error(res)) {
    res = json_object_new_object();
  }
  res_headers = parse_header_text(LBS_Body(headers));
  json_object_object_add(res, "headers", res_headers);
  curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
  json_object_object_add(res, "response_code", json_object_new_int(response_code));

  json_object_put(params);
  curl_slist_free_all(request_headers);
  curl_easy_reset(curl);

  FreeLBS(body);
  FreeLBS(headers);

  return res;
}

void doAuthenticationRequestToRP(OpenIdConnectProtocol *oip) {
  json_object *obj, *meta, *result, *headers;
  obj = json_object_new_object();
  meta = json_object_new_object();
  json_object_object_add(obj,"meta",meta);
  json_object_object_add(meta,"client_version",json_object_new_string(PANDA_CLIENT_VERSION));
  result = request(oip, oip->SP_URI, OPENID_HTTP_POST, obj, NULL);

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

  if (!json_object_object_get_ex(result, "headers", &headers)) {
    Error(_("no headers object"));
  }
  if (!json_object_object_get_ex(headers, "Location", &obj)) {
    Error(_("no Location object"));
  }
  oip->AuthenticationRequestURI = g_strdup(json_object_get_string(obj));

  json_object_put(result);
}

void doAuthenticationRequestToIp(OpenIdConnectProtocol *oip) {
  json_object *params, *result, *obj, *headers;
  char *redirect_uri;

  params = json_object_new_object();
  json_object_object_add(params, "response_type", json_object_new_string("code"));
  json_object_object_add(params, "scope", json_object_new_string("openid"));
  json_object_object_add(params, "client_id", json_object_new_string(oip->ClientId));
  json_object_object_add(params, "state", json_object_new_string(oip->State));
  json_object_object_add(params, "redirect_uri", json_object_new_string(oip->RedirectURI));
  json_object_object_add(params, "nonce", json_object_new_string(oip->Nonce));

  result = request(oip, oip->AuthenticationRequestURI, OPENID_HTTP_POST, params, NULL);

  json_object_object_get_ex(result, "headers", &headers);
  if (!json_object_object_get_ex(headers, "Location", &obj)) {
    Error(_("no Location object"));
  }
  redirect_uri = g_strdup(json_object_get_string(obj));

  params = json_object_new_object();
  result = request(oip, redirect_uri, OPENID_HTTP_GET, params, NULL);

  if (!json_object_object_get_ex(result, "request_url", &obj)) {
    Error(_("no request_url object"));
  }
  oip->RequestURL = g_strdup(json_object_get_string(obj));

  json_object_put(result);
}

void doLoginToIP(OpenIdConnectProtocol *oip) {
  json_object *params, *result, *obj, *headers;
  char *redirect_uri, *ip_cookie;
  params = json_object_new_object();
  json_object_object_add(params, "response_type", json_object_new_string("code"));
  json_object_object_add(params, "scope", json_object_new_string("openid"));
  json_object_object_add(params, "client_id", json_object_new_string(oip->ClientId));
  json_object_object_add(params, "state", json_object_new_string(oip->State));
  json_object_object_add(params, "redirect_uri", json_object_new_string(oip->RedirectURI));
  json_object_object_add(params, "nonce", json_object_new_string(oip->Nonce));
  json_object_object_add(params, "login_id", json_object_new_string(oip->User));
  json_object_object_add(params, "password", json_object_new_string(oip->Password));

  result = request(oip, oip->RequestURL, OPENID_HTTP_POST, params, NULL);

  json_object_object_get_ex(result, "response_code", &obj);
  if (json_object_get_int(obj) == 403) {
    Error(_("User ID or Password Incorrect"));
    exit(0);
  }

  json_object_object_get_ex(result, "headers", &headers);
  if (!json_object_object_get_ex(headers, "Location", &obj)) {
    Error(_("no Location object"));
  }
  redirect_uri = g_strdup(json_object_get_string(obj));

  if (!json_object_object_get_ex(headers, "Set-Cookie", &obj)) {
    Error(_("no Set-Cookie object"));
  }
  ip_cookie = g_strdup(json_object_get_string(obj));

  params = json_object_new_object();
  result = request(oip, redirect_uri, OPENID_HTTP_GET, params, ip_cookie);

  json_object_object_get_ex(result, "headers", &headers);
  if (!json_object_object_get_ex(headers, "Location", &obj)) {
    Error(_("no Location object"));
  }

  oip->GetSessionURI = g_strdup(json_object_get_string(obj));

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

extern void OpenIdConnectSetSSL(OpenIdConnectProtocol *oip, const char *CertFile, const char *CertKeyFile, const char *CertPass, const char *CAFile) {
  oip->CertFile = g_strdup(CertFile);
  oip->CertKeyFile= g_strdup(CertKeyFile);
  oip->CertPass = g_strdup(CertPass);
  oip->CAFile = g_strdup(CAFile);
}

extern void StartOpenIdConnect(OpenIdConnectProtocol *oip) {
  // バックエンドサーバへのログイン要求
  doAuthenticationRequestToRP(oip);
  // 認証サーバへのログイン要求
  doAuthenticationRequestToIp(oip);
  // 認証サーバへのログイン
  doLoginToIP(oip);
}
