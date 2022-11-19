/*
 * Most of this comes from FiSH sources, adapted for ScrollZ by flier
 *
 * $Id: fish.c,v 1.2 2009-12-21 14:14:17 f Exp $
 */

#include "irc.h"
#include "ircaux.h"
#include "list.h"
#include "vars.h"
#include "parse.h"
#include "output.h"

#ifdef HAVE_GMP
#include "fish.h"

/* #define S(x,i) (bf_S[i][x.w.byte##i]) */
#define S0(x) (bf_S[0][x.w.byte0])
#define S1(x) (bf_S[1][x.w.byte1])
#define S2(x) (bf_S[2][x.w.byte2])
#define S3(x) (bf_S[3][x.w.byte3])
#define bf_F(x) (((S0(x) + S1(x)) ^ S2(x)) + S3(x))
#define ROUND(a,b,n) (a.word ^= bf_F(b) ^ bf_P[n])

#define MAX_MSG_LEN 304

/* Public Base64 conversion tables */ 
unsigned char B64ABC[]="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
unsigned char b64buf[256];

extern char MyPrivKey[];
extern char MyPubKey[];
extern struct encrstr *encrlist;
extern void PrintUsage _((char *));

/* void initb64(); 
   Initializes the base64->base16 conversion tab. 
   Call this function once when your program starts.  */
void 
initb64 (void) {
    unsigned int i;

    for (i = 0; i < 256; i++) b64buf[i] = 0x00;
    for (i = 0; i < 64; i++) b64buf[(B64ABC[i])] = i;
}

/* int b64toh(lpBase64String, lpDestinationBuffer);
   Converts base64 string b to hexnumber d.
   Returns size of hexnumber in bytes.  */
int b64toh(char *b, char *d) {
    unsigned int i, k, l;

    l = strlen(b);
    if (l < 2) return 0;
    i = 0;
    k = 0;
    while (1) {
        i++;
        if (k + 1 < l) d[i - 1] = ((b64buf[(b[k])]) << 2);
        else break;
        k++;
        if (k < l) d[i - 1] |= ((b64buf[(b[k])] )>> 4);
        else break;
        i++;
        if (k + 1 < l) d[i - 1] = ((b64buf[(b[k])]) << 4);
        else break;
        k++;
        if (k < l) d[i - 1] |= ((b64buf[(b[k])]) >> 2);
        else break;
        i++;
        if (k + 1 < l) d[i - 1] = ((b64buf[(b[k])]) << 6);
        else break;
        k++;
        if (k < l) d[i - 1] |= (b64buf[(b[k])]);
        else break;
        k++;
    }
    return i - 1;
}

/* int htob64(lpHexNumber, lpDestinationBuffer);
   Converts hexnumber h (with length l bytes) to base64 string d.
   Returns length of base64 string.  */
int htob64(char *h, char *d, unsigned int l) {
    unsigned int i, j, k;
    unsigned char m, t;

    if (!l) return 0;
    l <<= 3;
    m = 0x80;
    for (i = 0, j = 0, k = 0, t = 0; i < l; i++) {
        if (h[(i >> 3)] & m) t |= 1;
        j++;
        if (!(m >>= 1)) m = 0x80;
        if (!(j % 6)) {
            d[k] = B64ABC[t];
            t &= 0;
            k++;
        }
        t <<= 1;
    }
    m = 5 - (j % 6);
    t <<= m;
    if (m) {
        d[k] = B64ABC[t];
        k++;
    }
    d[k] &= 0;
    return strlen(d);
}


/* blowfish stuff */
static u_32bit_t bf_P[bf_N + 2];
static u_32bit_t bf_S[4][256];

static void blowfish_encipher(u_32bit_t *xl, u_32bit_t *xr) {
    union aword Xl;
    union aword Xr;

    Xl.word = *xl;
    Xr.word = *xr;

    Xl.word ^= bf_P[0];
    ROUND(Xr, Xl, 1);
    ROUND(Xl, Xr, 2);
    ROUND(Xr, Xl, 3);
    ROUND(Xl, Xr, 4);
    ROUND(Xr, Xl, 5);
    ROUND(Xl, Xr, 6);
    ROUND(Xr, Xl, 7);
    ROUND(Xl, Xr, 8);
    ROUND(Xr, Xl, 9);
    ROUND(Xl, Xr, 10);
    ROUND(Xr, Xl, 11);
    ROUND(Xl, Xr, 12);
    ROUND(Xr, Xl, 13);
    ROUND(Xl, Xr, 14);
    ROUND(Xr, Xl, 15);
    ROUND(Xl, Xr, 16);
    Xr.word ^= bf_P[17];

    *xr = Xl.word;
    *xl = Xr.word;
}

static void blowfish_decipher(u_32bit_t *xl, u_32bit_t *xr) {
    union aword Xl;
    union aword Xr;

    Xl.word = *xl;
    Xr.word = *xr;

    Xl.word ^= bf_P[17];
    ROUND(Xr, Xl, 16);
    ROUND(Xl, Xr, 15);
    ROUND(Xr, Xl, 14);
    ROUND(Xl, Xr, 13);
    ROUND(Xr, Xl, 12);
    ROUND(Xl, Xr, 11);
    ROUND(Xr, Xl, 10);
    ROUND(Xl, Xr, 9);
    ROUND(Xr, Xl, 8);
    ROUND(Xl, Xr, 7);
    ROUND(Xr, Xl, 6);
    ROUND(Xl, Xr, 5);
    ROUND(Xr, Xl, 4);
    ROUND(Xl, Xr, 3);
    ROUND(Xr, Xl, 2);
    ROUND(Xl, Xr, 1);
    Xr.word ^= bf_P[0];

    *xl = Xr.word;
    *xr = Xl.word;
}

static void blowfish_init(u_8bit_t * key, int keybytes) {
    int i, j;
    u_32bit_t data;
    u_32bit_t datal;
    u_32bit_t datar;
    union aword temp;

    /* Fixes crash if key is longer than 80 char. This may cause the key
       to not end with \00 but that's no problem. */
    if (keybytes > 80) keybytes = 80;

    /* Reset blowfish boxes to initial state */
    for (i = 0; i < bf_N + 2; i++) bf_P[i] = initbf_P[i];
    for (i = 0; i < 4; i++)
        for (j = 0; j < 256; j++)
            bf_S[i][j] = initbf_S[i][j];

    j = 0;
    if (keybytes > 0) {
        for (i = 0; i < bf_N + 2; ++i) {
            temp.word = 0;
            temp.w.byte0 = key[j];
            temp.w.byte1 = key[(j + 1) % keybytes];
            temp.w.byte2 = key[(j + 2) % keybytes];
            temp.w.byte3 = key[(j + 3) % keybytes];
            data = temp.word;
            bf_P[i] = bf_P[i] ^ data;
            j = (j + 4) % keybytes;
        }
    }
    datal = 0x00000000;
    datar = 0x00000000;
    for (i = 0; i < bf_N + 2; i += 2) {
        blowfish_encipher(&datal, &datar);
        bf_P[i] = datal;
        bf_P[i + 1] = datar;
    }
    for (i = 0; i < 4; ++i) {
        for (j = 0; j < 256; j += 2) {
            blowfish_encipher(&datal, &datar);
            bf_S[i][j] = datal;
            bf_S[i][j + 1] = datar;
        }
    }
}

/* decode base64 string */
static int base64dec(char c) {
    int i;

    for (i = 0; i < 64; i++)
        if (B64[i] == c) return i;

    return 0;
}

/* Returned string must be freed when done with it! */
int encrypt_string(char *key, char *str, char *dest, int len) {
    u_32bit_t left, right;
    unsigned char *p;
    char *s, *d;
    int i;

    /* Pad fake string with 8 bytes to make sure there's enough */
    s = (char *) malloc(len + 9);
    strncpy(s, str, len);
    s[len]=0;
    if ((!key) || (!key[0])) {
		new_free(&s);
		return 0;
	}
    p = s;
    while (*p) p++;
    for (i = 0; i < 8; i++) *p++ = 0;
    blowfish_init((unsigned char *) key, strlen(key));
    p = s;
    d = dest;
    while (*p) {
        left = ((*p++) << 24);
        left += ((*p++) << 16);
        left += ((*p++) << 8);
        left += (*p++);
        right = ((*p++) << 24);
        right += ((*p++) << 16);
        right += ((*p++) << 8);
        right += (*p++);
        blowfish_encipher(&left, &right);
        for (i = 0; i < 6; i++) {
            *d++ = B64[right & 0x3f];
            right = (right >> 6);
        }
        for (i = 0; i < 6; i++) {
            *d++ = B64[left & 0x3f];
            left = (left >> 6);
        }
    }
    *d = 0;
    free(s);
    return 1;
}

int decrypt_string(char *key, char *str, char *dest, int len) {
    u_32bit_t left, right;
    char *p, *s, *d;
    int i;

    /* Pad encoded string with 0 bits in case it's bogus */
    if ((!key) || (!key[0])) return 0;
    s = (char *) malloc(len + 12);
    strncpy(s, str, len);
    s[len]=0;
    p = s;
    while (*p) p++;
    for (i = 0; i < 12; i++) *p++ = 0;
    blowfish_init((unsigned char *) key, strlen(key));
    p = s;
    d = dest;
    while (*p) {
        right = 0L;
        left = 0L;
        for (i = 0; i < 6; i++) right |= (base64dec(*p++)) << (i * 6);
        for (i = 0; i < 6; i++) left |= (base64dec(*p++)) << (i * 6);
        blowfish_decipher(&left, &right);
        for (i = 0; i < 4; i++) *d++ = (left & (0xff << ((3 - i) * 8))) >> ((3 - i) * 8);
        for (i = 0; i < 4; i++) *d++ = (right & (0xff << ((3 - i) * 8))) >> ((3 - i) * 8);
    }
    *d = 0;
    free(s);
    return 1;
}

void memXOR(unsigned char *s1, const unsigned char *s2, int n) {
    while (n--) *s1++ ^= *s2++;
}

/* Initiate DH 1080 key exchange */
void 
KeyExchange (char *command, char *args, char *subargs)
{
    char *target;
    char *pubkey = MyPubKey;
    char *privkey = MyPrivKey;
    char tmpbuf[mybufsize / 32];

    target = new_next_arg(args, &args);
    if (target) {
        if (is_channel(target)) {
            say("Key exchange does not work for channels");
            return;
        }
        DH1080_gen(privkey, pubkey);
        send_to_server("NOTICE %s :DH1080_INIT %s", target, MyPubKey);
    }
    else PrintUsage("KEYX nick");
}

/* Retrieve their key */
void 
FishAddRemoteKey (char *nick, char *notice)
{
    char pubkey[mybufsize / 2];
    struct encrstr *tmp;

    if (!strncmp(notice, "DH1080_FINISH ", 14)) {
        strmcpy(pubkey, notice + 14, sizeof(pubkey));
        if (DH1080_comp(MyPrivKey, pubkey) == 0) {
            say("Received non-matching key from %s", nick);
            return;
        }
        tmp = (struct encrstr *) list_lookup((List **) &encrlist, nick, !USE_WILDCARDS,
                !REMOVE_FROM_LIST); 
        if (tmp) {
            malloc_strcpy(&(tmp->key), pubkey);
            tmp->type = 2;
        }
        else {
            tmp = (struct encrstr *) new_malloc(sizeof(struct encrstr));
            tmp->next = (struct encrstr *) 0;
            tmp->user = (char *) 0; 
            tmp->key = (char *) 0;
            tmp->type = 2;
            malloc_strcpy(&(tmp->user), nick);
            malloc_strcpy(&(tmp->key), pubkey);
            add_to_list((List **) &encrlist, (List *) tmp);
        }
        say("Communication with %s will be encrypted using FiSH", nick);
    }
    else if (!strncmp(notice, "DH1080_INIT ", 12)) {
        strmcpy(pubkey, notice + 12, sizeof(pubkey));
        if (strspn(pubkey, B64ABC) != strlen(pubkey)) {
            say("Received non-matching key from %s", nick);
            return;
        }
        DH1080_gen(MyPrivKey, MyPubKey);
        if (DH1080_comp(MyPrivKey, pubkey) == 0) {
            say("Received non-matching key from %s", nick);
            return;
        }
        tmp = (struct encrstr *) list_lookup((List **) &encrlist, nick, !USE_WILDCARDS,
                !REMOVE_FROM_LIST); 
        if (tmp) {
            malloc_strcpy(&(tmp->key), pubkey);
            tmp->type = 2;
        }
        else {
            tmp = (struct encrstr *) new_malloc(sizeof(struct encrstr));
            tmp->next = (struct encrstr *) 0;
            tmp->user = (char *) 0; 
            tmp->key = (char *) 0;
            tmp->type = 2;
            malloc_strcpy(&(tmp->user), nick);
            malloc_strcpy(&(tmp->key), pubkey);
            add_to_list((List **) &encrlist, (List *) tmp);
        }
        say("Received DH1080 public key from %s, sending mine", nick);
        say("Communication with %s will be encrypted using FiSH", nick);
        send_to_server("NOTICE %s :DH1080_FINISH %s", nick, MyPubKey);
    }
}

int 
FishDecrypt (char *dest, char *src, char *key, int bufsize)
{
    int len;

    if (strncmp(src, "+OK ", 4)==0) src += 4;
    else if (strncmp(src, "mcps ", 5)==0) src += 5;
    len = strlen(src);
    if ((strspn(src, B64) != len) || (len < 12)) return(0);
    if (len >= bufsize) src[bufsize] = '\0';
    if (len != (len / 12) * 12) {
        len = (len / 12) * 12;
        src[len] = '\0';
    }
    decrypt_string(key, src, dest, len);
    if (*dest == '\0') return(0);
    return(2);
}

/* type = 0 ... private msg
          1 ... public msg */
int 
FishEncrypt (char *dest, char *src, char *key, int bufsize, int type)
{
    int len = strlen(src);
    int i = 0, j, padlen;
    int dopad = 1;
    char tmp[mybufsize];
    char msg[mybufsize];
    char *x = src;
    typedef struct { int len; char ch; } padstr;
#define NUM_PADCHARS 4
    padstr padchars[NUM_PADCHARS] = { { 1, ALL_OFF }, { 2, BOLD_TOG },
                                      { 2, REV_TOG }, { 2, UND_TOG  } };

    /* check if msg padding is enabled for given msg type */
    switch (type) {
        case SZ_ENCR_PRIVMSG:
            if (get_int_var(ENCRYPT_PAD_MSGS_VAR) == 0) dopad = 0;
            break;
        case SZ_ENCR_PUBLIC:
            if (get_int_var(ENCRYPT_PAD_PUBLIC_VAR) == 0) dopad = 0;
            break;
        default:
            dopad = 0;
            break;
    }
    /* pad the msg to max. allowed length by inserting a random
       sequence of invisible characters */
    if (dopad && (len > 0) && (len < MAX_MSG_LEN)) {
        memset(msg, 0, sizeof(msg));
        padlen = MAX_MSG_LEN - len;
        while ((padlen > 0) && (i < sizeof(msg) - 1)) {
            /* copy one character from src */
            if (*x) msg[i++] = *x++;
            if (i < sizeof(msg) - 1) {
                /* append a random pad character, if we have space for only
                   one character it has to be ALL_OFF */
                if (padlen == 1) j = 0;
                else j = rand() % NUM_PADCHARS;
                msg[i++] = padchars[j].ch;
                if (padchars[j].len == 2) msg[i++] = padchars[j].ch;
                padlen -= padchars[j].len;
            }
        }
        /* copy the rest of src */
        while (i < sizeof(msg) - 1) {
            if (*x) msg[i] = *x++;
            i++;
        }
        len = strlen(msg);
    }
    else strncpy(msg, src, sizeof(msg));
    if (sizeof(tmp) < len) len = sizeof(tmp);
    encrypt_string(key, msg, tmp, len);
    snprintf(dest, bufsize, "%s%s", SZBLOWSTR1, tmp);
    return(2);
}
#endif /* HAVE_GMP */
