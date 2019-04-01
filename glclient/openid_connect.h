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
} OpenIdConnectProtocol;

OpenIdConnectProtocol *InitOpenIdConnectProtocol(const char *sso_sp_uri, const char *user, const char *pass);
void StartOpenIdConnect(OpenIdConnectProtocol *);
