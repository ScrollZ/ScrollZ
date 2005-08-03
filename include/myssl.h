#ifndef _myssl_h_
#define _myssl_h_

#ifdef HAVE_SSL
#include <gnutls/gnutls.h>

#ifndef TRUE
#define TRUE 0 
#endif

#ifndef FALSE
#define FALSE 1
#endif

#define CHK_NULL(x) if ((x)==NULL) { say("SSL error - NULL data form server"); return; }
#define CHK_ERR(err, s) if ((err)==-1) { say("SSL prime error - %s", s); return; }
#define CHK_SSL(err) if ((err)==-1) { say("SSL error - %d", err); return; }

#endif /* SSL */

#endif /* _myssl_h_ */
