#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <dirent.h>
#include <uuid/uuid.h>
#include <time.h>
#include <libgen.h>
#include <gtk/gtk.h>
#include <gtkpanda/gtkpanda.h>
#include <openssl/pkcs12.h>
#include <openssl/x509.h>
#include <openssl/pem.h>

#include "certificate.h"
#include "glclient.h"
#include "protocol.h"
#include "bd_config.h"
#include "const.h"
#include "widgetcache.h"
#include "desktop.h"
#include "bootdialog.h"
#include "action.h"
#include "notify.h"
#include "gettext.h"
#include "logger.h"
#include "utils.h"
#include "tempdir.h"
#include "dialogs.h"

#define CERT_EXPIRE_CHECK_MONTHES 2

#define CERT_HTTP_GET 1
#define CERT_HTTP_POST 2

size_t cert_write_data(void *buf, size_t size, size_t nmemb, void *userp) {
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

size_t cert_write_data_to_fp(void *buf, size_t size, size_t nmemb, void *userp) {
  size_t buf_size = size * nmemb;
  FILE *fp;

  fp = (FILE *)userp;
  fwrite(buf, buf_size, 1, fp);
  return buf_size;
}

json_object *cert_request(CURL *curl, char *uri, int method, char *filename) {
  LargeByteString *body, *headers;
  json_object *res;
  struct curl_slist *request_headers = NULL;
  FILE *fp;
  int response_code;

  body = NewLBS();
  headers = NewLBS();

  if (filename != NULL) {
    if ((fp = fopen(filename, "wb")) == NULL) {
      Warning("file open failure");
      exit(1);
    }
  }

  curl_easy_setopt(curl, CURLOPT_URL, uri);
  if (filename == NULL) {
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)body);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, cert_write_data);
  } else {
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)fp);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, cert_write_data_to_fp);
  }
  curl_easy_setopt(curl, CURLOPT_HEADERDATA, (void *)headers);
  curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, cert_write_data);
  request_headers = curl_slist_append(request_headers, "Accept: application/json");
  if (method == CERT_HTTP_POST) {
    request_headers = curl_slist_append(request_headers, "Content-Type: application/json");
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, "");
  }
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, request_headers);

  curl_easy_perform(curl);
  curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
  Info("code: %d", response_code);
  if (response_code != 200) {
    return NULL;
  }

  if (filename == NULL) {
    res = json_tokener_parse(LBS_Body(body));
  } else {
    res = NULL;
  }
  if (res == NULL) {
    res = json_object_new_object();
  }

  curl_slist_free_all(request_headers);
  curl_easy_reset(curl);

  if (filename != NULL) {
    fclose(fp);
  }

  return res;
}
void initCertDir(Certificate *cert) {
  gchar *dir;
  dir = g_build_filename(g_get_home_dir(), GL_ROOT_DIR, "certificates", NULL);
  MakeDir(dir, 0700);
  cert->cert_dir = dir;
}

void setupNewCert(Certificate *cert) {
  time_t now = time(NULL);
  char buf[256];
  strftime(buf, 256, "%Y%m%d%H%M%S.crt", localtime(&now));
  cert->new_cert_name = g_build_filename(cert->cert_dir, buf, NULL);
  strftime(buf, 256, "%Y%m%d%H%M%S.p12", localtime(&now));
  cert->new_p12_name = g_build_filename(cert->cert_dir, buf, NULL);
}

Certificate *initCertificate() {
  Certificate *cert;
  cert = g_new0(Certificate, 1);
  cert->Curl = curl_easy_init();
  if (!cert->Curl) {
    Warning("curl_easy_init failure");
    exit(1);
  }
  return cert;
}

void cert_setSSL(Certificate *cert) {
  curl_easy_setopt(cert->Curl, CURLOPT_USE_SSL, CURLUSESSL_ALL);
  curl_easy_setopt(cert->Curl, CURLOPT_SSL_VERIFYPEER, 1);
  curl_easy_setopt(cert->Curl, CURLOPT_SSL_VERIFYHOST, 2);
  if (strlen(cert->CertFile) > 0 && strlen(cert->CertKeyFile) > 0) {
    curl_easy_setopt(cert->Curl, CURLOPT_SSLCERT, cert->CertFile);
    curl_easy_setopt(cert->Curl, CURLOPT_SSLCERTTYPE, "PEM");
    curl_easy_setopt(cert->Curl, CURLOPT_SSLKEY, cert->CertKeyFile);
    curl_easy_setopt(cert->Curl, CURLOPT_SSLKEYTYPE, "PEM");
    curl_easy_setopt(cert->Curl, CURLOPT_SSLKEYPASSWD, cert->CertPass);
  }
  if (cert->CAFile == NULL || strlen(cert->CAFile) <= 0) {
    Error("set CAFile option");
  }
  curl_easy_setopt(cert->Curl, CURLOPT_CAINFO, cert->CAFile);
}

int call_update_certificate(Certificate *cert) {
  char url_buf[256];
  json_object *result, *obj;
  sprintf(url_buf, "%s/api/cert", cert->APIDomain);
  result = cert_request(cert->Curl, url_buf, CERT_HTTP_POST, NULL);

  if (result == NULL) {
    return -1;
  }

  if (!json_object_object_get_ex(result, "uri", &obj)) {
    Error(_("no uri object"));
  }
  cert->GetURI = g_strdup(json_object_get_string(obj));

  if (!json_object_object_get_ex(result, "pass", &obj)) {
    Error(_("no pass object"));
  }
  cert->NewCertPass = g_strdup(json_object_get_string(obj));
  return 0;
}

void get_new_certificate(Certificate *cert) {
  cert_request(cert->Curl, cert->GetURI, CERT_HTTP_GET, cert->new_p12_name);
}

gchar *extract_domain(const char *AuthURI) {
  gchar *uri = g_strdup(AuthURI);
  int i = strstr(uri, "//") - uri + 2;
  while(uri[i] != '/') { i++; }
  i++;
  uri[i] = '\0';
  return uri;
}

void decode_p12(Certificate *cert) {
  FILE *fp;
  EVP_PKEY *pkey;
  X509 *x509 = X509_new();
  STACK_OF(X509) *ca = NULL;
  PKCS12 *p12;
  if ((fp = fopen(cert->new_p12_name, "rb")) == NULL) {
    Error("Error open file");
  }

  p12 = d2i_PKCS12_fp(fp, NULL);
  fclose(fp);
  if (!p12) {
    Error("Error reading PKCS#12 file");
  }
  if (!PKCS12_parse(p12, cert->NewCertPass, &pkey, &x509, &ca)) {
    Error("Error parsing PKCS#12 file");
  }
  PKCS12_free(p12);
  fclose(fp);

  if ((fp = fopen(cert->new_cert_name, "w")) == NULL) {
    Error("Error open file");
  }
  PEM_write_X509_AUX(fp, x509);
  fclose(fp);
}

void save_cert_config(Certificate *cert) {
  int n = gl_config_get_index();
  gl_config_set_string(n, "certfile", cert->new_cert_name);
  gl_config_save();
  Info("saved: %d, %s", n, cert->new_cert_name);
}

void updateCertificate(const char *AuthURI, const char *CertFile, const char *CertKeyFile, const char *CertPass, const char *CAFile) {
  Certificate *cert = initCertificate();
  cert->APIDomain = extract_domain(AuthURI);
  cert->CertFile = CertFile;
  cert->CertKeyFile = CertKeyFile;
  cert->CertPass = CertPass;
  cert->CAFile = CAFile;
  cert_setSSL(cert);
  initCertDir(cert);
  if(call_update_certificate(cert) < 0) {
    MessageDialog(GTK_MESSAGE_WARNING, _("Failure update certificate."));
    return;
  }
  setupNewCert(cert);
  get_new_certificate(cert);
  decode_p12(cert);
  save_cert_config(cert);
  return;
}

void checkCertificateExpire(const char *AuthURI, const char *CertFile, const char *CertKeyFile, const char *CertPass, const char *CAFile) {
  FILE *fp;
  X509 *cert;
  ASN1_TIME *not_after;
  int day, sec;

  if ((fp = fopen(CertFile, "rb")) == NULL) {
    Error(_("does not open cert"));
  }
  cert = PEM_read_X509_AUX(fp, NULL, NULL, NULL);
  if (cert == NULL) {
    Error(_("cert read failure"));
  }

  not_after = X509_get_notAfter(cert);
  ASN1_TIME_diff(&day, &sec, NULL, not_after);

  if (day < (CERT_EXPIRE_CHECK_MONTHES * 30)) {
    time_t t = time(NULL);
    char s[256];
    t += day * 24 * 3600 + sec;
    strftime(s, sizeof(s), _("Certificate Expiration is Approaching.\nexpiration: %Y/%m/%d/ %H:%M:%S"), localtime(&t));
    if (ConfirmDialog(s)) {
      updateCertificate(AuthURI, CertFile, CertKeyFile, CertPass, CAFile);
    }
  }
}
