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

void checkCertificateExpire(const char *file) {
  FILE *fp;
  X509 *cert;
  ASN1_TIME *not_after;
  int day, sec;

  if ((fp = fopen(file, "rb")) == NULL) {
    Error("Error open file");
  }
  cert = PEM_read_X509(fp, NULL, NULL, NULL);

  not_after = X509_get_notAfter(cert);
  ASN1_TIME_diff(&day, &sec, NULL, not_after);

  if (day < (CERT_EXPIRE_CHECK_MONTHES * 30)) {
    time_t t = time(NULL);
    char s[256];
    t += day * 24 * 3600 + sec;
    strftime(s, sizeof(s), _("Certificate Expiration is Approaching.\nexpiration: %Y/%m/%d/ %H:%M:%S"), localtime(&t));
    MessageDialog(GTK_MESSAGE_INFO, s);
  }
}
