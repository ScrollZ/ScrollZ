#ifndef _myssl_h_
#define _myssl_h_

#if defined(HAVE_SSL) || defined(HAVE_OPENSSL)

#if defined(HAVE_SSL)
#include <gnutls/gnutls.h>
#elif defined(HAVE_OPENSSL)
#include <openssl/crypto.h>
#include <openssl/x509.h>
#include <openssl/pem.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#endif

#ifndef TRUE
#define TRUE 0 
#endif

#ifndef FALSE
#define FALSE 1
#endif

#define CHK_NULL(x) if ((x)==NULL) { say("SSL error - NULL data form server"); return; }
#define CHK_ERR(err, s) if ((err)==-1) { say("SSL prime error - %s", s); return; }
#define CHK_SSL(err) if ((err)==-1) { say("SSL error - %d", err); return; }

#endif /* SSL || HAVE_SSL */

#endif /* _myssl_h_ */
