#ifndef _INC_OPENID_CONNECT_H
#define _INC_OPENID_CONNECT_H

#include <json.h>
#include <curl/curl.h>

typedef struct {
  CURL *Curl;
  char *User;
  char *Password;
  char *SP_URI;

  char *ClientId;
  char *State;
  char *RedirectURI;
  char *Nonce;
  char *AuthenticationRequestURI;
  char *RequestURL;
  char *GetSessionURI;
  char *SessionID;
  gboolean fSSL;
  char *CAFile;
  char *CertFile;
  char *CertKeyFile;
  char *CertPass;
} OpenIdConnectProtocol;

OpenIdConnectProtocol *InitOpenIdConnectProtocol(const char *sso_sp_uri, const char *user, const char *pass);
void StartOpenIdConnect(OpenIdConnectProtocol *);
void OpenIdConnectSetSSL(OpenIdConnectProtocol *oip, const char *CertFile, const char *CertKeyFile, const char *CertPass, const char *CAFile);

#endif
