/*
 * Most of this is from FiSH, adapted for ScrollZ by flier
 *
 * $Id: dh1080.c,v 1.2 2009-12-21 14:14:17 f Exp $
 */

/* New Diffie-Hellman 1080bit Key-exchange */

/* For Diffie-Hellman key-exchange a 1080bit germain prime is used, the
   generator g=2 renders a field Fp from 1 to p-1. Therefore breaking it
   means to solve a discrete logarithm problem with no less than 1080bit.

   Base64 format is used to send the public keys over IRC.

   The calculated secret key is hashed with SHA-256, the result is converted
   to base64 for final use with blowfish. */

#include <string.h>
#include <time.h>
#include "irc.h"

#ifdef HAVE_GMP

#include "dh1080.h"

#define ZeroMemory(x,y) memset(&x, 0, y);

extern char *OpenCreateFile(char *, int);

static const char prime1080[135] = 
{
    0xFB, 0xE1, 0x02, 0x2E, 0x23, 0xD2, 0x13, 0xE8, 0xAC, 0xFA, 0x9A, 0xE8, 0xB9, 0xDF,
    0xAD, 0xA3, 0xEA, 0x6B, 0x7A, 0xC7, 0xA7, 0xB7, 0xE9, 0x5A, 0xB5, 0xEB, 0x2D, 0xF8,
    0x58, 0x92, 0x1F, 0xEA, 0xDE, 0x95, 0xE6, 0xAC, 0x7B, 0xE7, 0xDE, 0x6A, 0xDB, 0xAB,
    0x8A, 0x78, 0x3E, 0x7A, 0xF7, 0xA7, 0xFA, 0x6A, 0x2B, 0x7B, 0xEB, 0x1E, 0x72, 0xEA,
    0xE2, 0xB7, 0x2F, 0x9F, 0xA2, 0xBF, 0xB2, 0xA2, 0xEF, 0xBE, 0xFA, 0xC8, 0x68, 0xBA,
    0xDB, 0x3E, 0x82, 0x8F, 0xA8, 0xBA, 0xDF, 0xAD, 0xA3, 0xE4, 0xCC, 0x1B, 0xE7, 0xE8,
    0xAF, 0xE8, 0x5E, 0x96, 0x98, 0xA7, 0x83, 0xEB, 0x68, 0xFA, 0x07, 0xA7, 0x7A, 0xB6,
    0xAD, 0x7B, 0xEB, 0x61, 0x8A, 0xCF, 0x9C, 0xA2, 0x89, 0x7E, 0xB2, 0x8A, 0x61, 0x89,
    0xEF, 0xA0, 0x7A, 0xB9, 0x9A, 0x8A, 0x7F, 0xA9, 0xAE, 0x29, 0x9E, 0xFA, 0x7B, 0xA6,
    0x6D, 0xEA, 0xFE, 0xFB, 0xEF, 0xBF, 0x0B, 0x7D, 0x8B 
};

/* Input:  priv_key = buffer of 200 bytes
           pub_key  = buffer of 200 bytes
   Output: priv_key = Your private key
           pub_key  = Your public key */
void DH1080_gen(char *priv_key, char *pub_key)
{
    unsigned char raw_buf[160], iniHash[33];
    char *filepath;
    FILE *hRnd;
    size_t len;

    mpz_t b_privkey, b_prime1080, b_pubkey, b_base;

    memset(priv_key, 0, 200);
    memset(pub_key, 0, 200);

    hRnd = fopen("/dev/urandom", "r"); /* don't use /dev/random, it's a blocking device */
    if (!hRnd) return;

    initb64();

    mpz_init(b_prime1080);
    mpz_import(b_prime1080, 135, 1, 1, 0, 0, prime1080);
    mpz_init(b_privkey);
    mpz_init(b_pubkey);
    mpz_init_set_ui(b_base, 2);

    do {
        unsigned char temp[135];
        fread(temp, 1, sizeof(temp), hRnd);
        mpz_import(b_privkey, 135, 1, 1, 0, 0, temp);
        mpz_mod(b_privkey, b_privkey, b_prime1080); /* [2, prime1080-1] */
    } while( mpz_cmp_ui(b_privkey, 1) != 1); /* while smaller than 2 */
    fclose(hRnd);

    mpz_powm(b_pubkey, b_base, b_privkey, b_prime1080);

    mpz_export(raw_buf, &len, 1, 1, 0, 0, b_privkey);
    mpz_clear(b_privkey);
    htob64(raw_buf, priv_key, len);

    mpz_export(raw_buf, &len, 1, 1, 0, 0, b_pubkey);
    htob64(raw_buf, pub_key, len);
    mpz_clear(b_pubkey);
    mpz_clear(b_base);
    mpz_clear(b_prime1080);
}



/* Input:  MyPrivKey = Your private key
           HisPubKey = Someones public key
   Output: MyPrivKey has been destroyed for security reasons
           HisPubKey = the secret key */
int DH1080_comp(char *MyPrivKey, char *HisPubKey)
{
    int i = 0, len;
    unsigned char SHA256digest[35], base64_tmp[160];
    mpz_t b_myPrivkey, b_HisPubkey, b_prime1080, b_theKey;

    /* Verify base64 strings */
    if ((strspn(MyPrivKey, B64ABC) != strlen(MyPrivKey)) || (strspn(HisPubKey, B64ABC) != strlen(HisPubKey)))
    {
        memset(MyPrivKey, 0x20, strlen(MyPrivKey));
        memset(HisPubKey, 0x20, strlen(HisPubKey));
        return 0;
    }

    mpz_init(b_prime1080);
    mpz_import(b_prime1080, 135, 1, 1, 0, 0, prime1080);

    mpz_init(b_myPrivkey);
    mpz_init(b_HisPubkey);
    mpz_init(b_theKey);

    len=b64toh(HisPubKey, base64_tmp);
    mpz_import(b_HisPubkey, len, 1, 1, 0, 0, base64_tmp);

    len=b64toh(MyPrivKey, base64_tmp);
    mpz_import(b_myPrivkey, len, 1, 1, 0, 0, base64_tmp);
    memset(MyPrivKey, 0x20, strlen(MyPrivKey));

    mpz_powm(b_theKey, b_HisPubkey, b_myPrivkey, b_prime1080);
    mpz_clear(b_myPrivkey);

    mpz_export(base64_tmp, &len, 1, 1, 0, 0, b_theKey);
    SHA256_memory(base64_tmp, len, SHA256digest);
    htob64(SHA256digest, HisPubKey, 32);

    ZeroMemory(base64_tmp, sizeof(base64_tmp));
    ZeroMemory(SHA256digest, sizeof(SHA256digest));

    mpz_clear(b_theKey);
    mpz_clear(b_HisPubkey);

    return 1;
}

#endif /* HAVE_GMP */
