#include <curl/curl.h>
extern void checkCertificateExpire(const char *AuthURI, const char *CertFile, const char *CertKeyFile, const char *CertPass, const char *CAFile);

typedef struct {
  CURL *Curl;
  gchar *cert_dir;
  gchar *new_cert_name;
  gchar *new_p12_name;
  gchar *APIDomain;
  char *CertFile;
  char *CertKeyFile;
  char *CertPass;
  char *CAFile;
  char *GetURI;
  char *NewCertPass;
} Certificate;
