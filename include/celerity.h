/* Small header file for use by Celerity.
 *
 * Could be put elsewhere but for now...
 *
 *          - j. roethel [03.06.98]
 *          
 * $Id: celerity.h,v 1.2 1998-10-31 18:26:42 f Exp $
 */

#ifndef _CELERITY_H_
#define _CELERITY_H_

#ifdef CELE
#define CELECOSM 1
#define CELEPROT 1
#define CELEAPPR 1
#define CELESCRP 1
#undef CELEHOOK
#define FORCE_HASCII 1
#else
#undef CELECOSM
#undef CELEPROT
#undef CELEAPPR
#undef CELESCRP
#undef CELEHOOK
#endif

#endif /* _CELERITY_H_ */
