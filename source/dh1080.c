/*
 * Most of this is from FiSH, adapted for ScrollZ by flier
 *
 * $Id: dh1080.c,v 1.1 2008-03-08 15:22:13 f Exp $
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

#ifdef HAVE_MIRACL
#include "dh1080.h"

#define ZeroMemory(x,y) memset(&x, 0, y);

extern char *OpenCreateFile(char *, int);

/* ### new sophie-germain 1080bit prime number ### */
char *prime1080="++ECLiPSE+is+proud+to+present+latest+FiSH+release+featuring+even+more+security+for+you+++shouts+go+out+to+TMG+for+helping+to+generate+this+cool+sophie+germain+prime+number++++/C32L";
/* Base16: FBE1022E23D213E8ACFA9AE8B9DFADA3EA6B7AC7A7B7E95AB5EB2DF858921FEADE95E6AC7BE7DE6ADBAB8A783E7AF7A7FA6A2B7BEB1E72EAE2B72F9FA2BFB2A2EFBEFAC868BADB3E828FA8BADFADA3E4CC1BE7E8AFE85E9698A783EB68FA07A77AB6AD7BEB618ACF9CA2897EB28A6189EFA07AB99A8A7FA9AE299EFA7BA66DEAFEFBEFBF0B7D8B */
/* Base10: 12745216229761186769575009943944198619149164746831579719941140425076456621824834322853258804883232842877311723249782818608677050956745409379781245497526069657222703636504651898833151008222772087491045206203033063108075098874712912417029101508315117935752962862335062591404043092163187352352197487303798807791605274487594646923 */


/* Input:  priv_key = buffer of 200 bytes
           pub_key  = buffer of 200 bytes
   Output: priv_key = Your private key
           pub_key  = Your public key */
void DH1080_gen(char *priv_key, char *pub_key)
{
    unsigned char raw_buf[160], iniHash[33];
    unsigned long seed;
    char *filepath;
    FILE *hRnd;
    int len;

    big b_privkey, b_prime, b_pubkey;
    csprng myRNG;

    priv_key[0]='0';
    priv_key[1]='\0';
    pub_key[0]='0';
    pub_key[1]='\0';

    hRnd = fopen("/dev/urandom", "r");  // don't use /dev/random, it's a blocking device
    if (!hRnd) return;

    initb64();
    mip = mirsys(500, 16);

    b_privkey = mirvar(0);
    b_pubkey = mirvar(0);
    b_prime = mirvar(0);

    len = b64toh(prime1080, raw_buf);
    bytes_to_big(len, raw_buf, b_prime);


    /* #*#*#*#*#* RNG START #*#*#*#*#* */
    time((time_t *) &seed);
    seed ^= (int) hRnd << 16;
    if(fread(raw_buf, 1, sizeof(raw_buf), hRnd) < 32) {
        ZeroMemory(raw_buf, sizeof(raw_buf));
        fclose(hRnd);
        mirkill(b_privkey);
        mirkill(b_pubkey);
        mirkill(b_prime);
        mirexit();
        return;
    }
    fclose(hRnd);
    filepath = OpenCreateFile("ScrollZ.save",0);
    sha_file(filepath, iniHash);
    memXOR(raw_buf + 128, iniHash, 32);
    ZeroMemory(iniHash, sizeof(iniHash));
    /* first 128 byte in raw_buf: output from ECL_PRNG.lib (linux: /dev/random)
       last 32 byte in raw_buf: SHA-256 digest from blow.ini */

    seed *= (int) mip;
    strong_init(&myRNG, sizeof(raw_buf), raw_buf, seed);
    strong_rng(&myRNG);
    strong_bigdig(&myRNG, 1080, 2, b_privkey);
    strong_kill(&myRNG);
    seed = 0;
    /* #*#*#*#*#* RNG END #*#*#*#*#* */


    powltr(2, b_privkey, b_prime, b_pubkey);

    len = big_to_bytes(sizeof(raw_buf), b_privkey, raw_buf, FALSE);
    mirkill(b_privkey);
    htob64(raw_buf, priv_key, len);

    len = big_to_bytes(sizeof(raw_buf), b_pubkey, raw_buf, FALSE);
    htob64(raw_buf, pub_key, len);
    ZeroMemory(raw_buf, sizeof(raw_buf));

    mirkill(b_pubkey);
    mirkill(b_prime);
}



/* Input:  MyPrivKey = Your private key
           HisPubKey = Someones public key
   Output: MyPrivKey has been destroyed for security reasons
           HisPubKey = the secret key */
BOOL DH1080_comp(char *MyPrivKey, char *HisPubKey)
{
    int i = 0, len;
    unsigned char SHA256digest[35], base64_tmp[160];
    big b_myPrivkey, b_HisPubkey, b_prime, b_theKey;

    /* Verify base64 strings */
    if ((strspn(MyPrivKey, B64ABC) != strlen(MyPrivKey)) || (strspn(HisPubKey, B64ABC) != strlen(HisPubKey)))
    {
        memset(MyPrivKey, 0x20, strlen(MyPrivKey));
        memset(HisPubKey, 0x20, strlen(HisPubKey));
        return 0;
    }

    b_myPrivkey = mirvar(0);
    b_HisPubkey = mirvar(0);
    b_theKey = mirvar(0);
    b_prime = mirvar(0);

    len = b64toh(prime1080, base64_tmp);
    bytes_to_big(len, base64_tmp, b_prime);

    len = b64toh(MyPrivKey, base64_tmp);
    bytes_to_big(len, base64_tmp, b_myPrivkey);
    memset(MyPrivKey, 0x20, strlen(MyPrivKey));

    len = b64toh(HisPubKey, base64_tmp);
    bytes_to_big(len, base64_tmp, b_HisPubkey);

    powmod(b_HisPubkey, b_myPrivkey, b_prime, b_theKey);
    mirkill(b_myPrivkey);

    len = big_to_bytes(sizeof(base64_tmp), b_theKey, base64_tmp, FALSE);
    mirkill(b_theKey);
    SHA256_memory(base64_tmp, len, SHA256digest);
    htob64(SHA256digest, HisPubKey, 32);
    ZeroMemory(base64_tmp, sizeof(base64_tmp));
    ZeroMemory(SHA256digest, sizeof(SHA256digest));

    mirkill(b_HisPubkey);
    mirkill(b_prime);
    return 1;
}
#endif /* HAVE_MIRACL */
