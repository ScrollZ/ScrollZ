/*
 * blowfish.c - Encryption/Decryption routines
 * Copyright (C) 1998 Goran Koruga (justme@neurocorp.net)
 *
 * Most of this code was written by Bruce Schneier (schneier@chixxx.com)
 *
 * Modified by Flier 1998 for ScrollZ
 *
 * Routines for encryption
 *
 * $Id: blowfish.c,v 1.11 2001-11-17 10:57:44 f Exp $
 */

#include "irc.h"
#include "ircaux.h"
#include "blowfish.h"

#define NUMPBOX       16
#define NUMSBOX       2
#define SZCRYPTSTROLD "++SZ"
#define SZCRYPTSTR    "+/SZ"
#define SZOLDCRYPT    '-'

static char *base64="./0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";

static char encrbuf[BIG_BUFFER_SIZE+1];

static unsigned int pbox[NUMPBOX+2];
static unsigned int sbox[NUMSBOX][256];

static unsigned int PBOX[NUMPBOX+2]={
    0x243A6A88, 0x85C308D3, 0x17198A2E, 0x03787344, 0xA9093822, 0x499F31D0,
    0x082E6A98, 0xEC3E6C89, 0xC52821E6, 0x38D013F7, 0xBE5E66CF, 0x54E90C6C,
    0xC0A329B7, 0xC57C50DD, 0x3F84DDB5, 0xB5440917, 0x9218D5D9, 0x8379FB1B
};
static unsigned int SBOX[NUMSBOX][256]={
    {
        0xA1310BA6, 0x38DFB5AC, 0x4FFD72DB, 0x501ADFB7, 0xA8E1AFED, 0x6A257E96,
        0xBA7C9045, 0xF12C7F99, 0x24A19947, 0xB3916CF7, 0x0801F2E2, 0x858EFC16,
        0x636920D8, 0x71574E69, 0xA458FEA3, 0xF4933D7E, 0x0D95748F, 0x728EB658,
        0x718BCD58, 0x82154AEE, 0x7B54A41D, 0xC25A59B5, 0x9C30D539, 0x2AF26013,
        0xC5D1B023, 0x286085F0, 0xCA417918, 0xB8DB38EF, 0x8E79DCB0, 0x603A180E,
        0x6C9E0E8B, 0xB01E8A3E, 0xD71577C1, 0xBD314B27, 0x78AF2FDA, 0x55605C60,
        0xE65525F3, 0xAA55AB94, 0x57489862, 0x63E81440, 0x55CA396A, 0x2AAB10B6,
        0xB4CC5C34, 0x1141E8CE, 0xA15486AF, 0x7C72E993, 0xB3EE1411, 0x636FBC2A,
        0x2BA9C55D, 0x741831F6, 0xCE5C3E16, 0x9B87931E, 0xAFD6BA33, 0x6C24CF5C,
        0x7A325381, 0x28958677, 0x3B8F4898, 0x6B4BB9AF, 0xC4BFE81B, 0x66282193,
        0x61D809CC, 0xFB21A991, 0x487CAC60, 0x5DEC8032, 0xEF845D5D, 0xE98575B1,
        0xDC262302, 0xEB651B88, 0x23893E81, 0xD396ACC5, 0x0F6D6FF3, 0x83F44239,
        0x2E0B4482, 0xA4842004, 0x69C8F04A, 0x9E1F9B5E, 0x21C66842, 0xF6E96C9A,
        0x670C9C61, 0xABD388F0, 0x6A51A0D2, 0xD8542F68, 0x960FA728, 0xAB5133A3,
        0x6EEF0B6C, 0x137A3BE4, 0xBA3BF050, 0x7EFB2A98, 0xA1F1651D, 0x39AF0176,
        0x66CA593E, 0x82430E88, 0x8CEE8619, 0x456F9FB4, 0x7D84A5C3, 0x3B8B5EBE,
        0xE06F75D8, 0x85C12073, 0x401A449F, 0x56C16AA6, 0x4ED3AA62, 0x363F7706,
        0x1BFEDF72, 0x429B023D, 0x37D0D724, 0xD00A1248, 0xDB0FEAD3, 0x49F1C09B,
        0x075372C9, 0x80991B7B, 0x25D479D8, 0xF6E8DEF7, 0xE3FE501A, 0xB6794C3B,
        0x976CE0BD, 0x04C006BA, 0xC1A94FB6, 0x409F60C4, 0x5E5C9EC2, 0x196A2463,
        0x68FB6FAF, 0x3E6C53B5, 0x1339B2EB, 0x3B52EC6F, 0x6DFC511F, 0x9B30952C,
        0xCC814544, 0xAF5EBD09, 0xBEE3D004, 0xDE334AFD, 0x660F2807, 0x192E4BB3,
        0xC0CBA857, 0x45C8740F, 0xD20B5F39, 0xB9D3FBDB, 0x5579C0BD, 0x1A60320A,
        0xD6A100C6, 0x402C7279, 0x679F25FE, 0xFB1FA3CC, 0x8EA5E9F8, 0xDB3222F8,
        0x3C7516DF, 0xFD616B15, 0x2F501EC8, 0xAD0552AB, 0x323DB5FA, 0xFD238760,
        0x53317B48, 0x3E00DF82, 0x9E5C57BB, 0xCA6F8CA0, 0x1A87562E, 0xDF1769DB,
        0xD542A8F6, 0x287EFFC3, 0xAC6732C6, 0x8C4F5573, 0x695B27B0, 0xBBCA58C8,
        0xE1FFA35D, 0xB8F011A0, 0x10FA3D98, 0xFD2183B8, 0x4AFCB56C, 0x2DD1D35B,
        0x9A53E479, 0xB6F84565, 0xD28E49BC, 0x4BFB9790, 0xE1DDF2DA, 0xA4CB7E33,
        0x62FB1341, 0xCEE4C6E8, 0xEF20CADA, 0x36774C01, 0xD07E9EFE, 0x2BF11FB4,
        0x95DBDA4D, 0xAE909198, 0xEAAD8E71, 0x6B93D5A0, 0xD08ED1D0, 0xAFC725E0,
        0x8E3C5B2F, 0x8E7594B7, 0x8FF6E2FB, 0xF2122B64, 0x8888B812, 0x900DF01C,
        0x4FAD5EA0, 0x688FC31C, 0xD1CFF191, 0xB3A8C1AD, 0x2F2F2218, 0xBE0E1777,
        0xEA752DFE, 0x8B021FA1, 0xE5A0CC0F, 0xB56F74E8, 0x18ACF3D6, 0xCE89E299,
        0xB4A84FE0, 0xFD13E0B7, 0x7CC43B81, 0xD2ADA8D9, 0x165FA266, 0x80957705,
        0x93CC7314, 0x211A1477, 0xE6AD2065, 0x77B5FA86, 0xC75442F5, 0xFB9D35CF,
        0xEBCDAF0C, 0x7B3E89A0, 0xD6411BD3, 0xAE1E7E49, 0x00250E2D, 0x2071B35E,
        0x226800BB, 0x57B8E0AF, 0x2464369B, 0xF009B91E, 0x5563911D, 0x59DFA6AA,
        0x78C14389, 0xD95A537F, 0x207D5BA2, 0x02E5B9C5, 0x83260376, 0x6295CFA9,
        0x11C81968, 0x4E734A41, 0xB3472DCA, 0x7B14A94A, 0x1B510052, 0x9A532915,
        0xD60F573F, 0xBC9BC6E4, 0x2B60A476, 0x81E67400, 0x08BA6FB5, 0x571BE91F,
        0xF296EC6B, 0x2A0DD915, 0xB6636521, 0xE7B9F9B6, 0xFF34052E, 0xC5855664,
        0x53B02D5D, 0xA99F8FA1, 0x08BA4799, 0x6E85076A
    } ,
    {
        0x3A39CE37, 0xD3FAF5CF, 0xABC27737, 0x5AC52D1B, 0x5CB0679E, 0x4FA33742,
        0xD3822740, 0x99BC9BBE, 0xD5118E9D, 0xBF0F7315, 0xD62D1C7E, 0xC700C47B,
        0xB78C1B6B, 0x21A19045, 0xB26EB1BE, 0x6A366EB4, 0x5748AB2F, 0xBC946E79,
        0xC6A376D2, 0x6549C2C8, 0x530FF8EE, 0x468DDE7D, 0xD5730A1D, 0x4CD04DC6,
        0x2939BBDB, 0xA9BA4650, 0xAC9526E8, 0xBE5EE304, 0xA1FAD5F0, 0x6A2D519A,
        0x63EF8CE2, 0x9A86EE22, 0xC089C2B8, 0x43242EF6, 0xA51E03AA, 0x9CF2D0A4,
        0x83C061BA, 0x9BE96A4D, 0x8FE51550, 0xBA645BD6, 0x2826A2F9, 0xA73A3AE1,
        0x4BA99586, 0xEF5562E9, 0xC72FEFD3, 0xF752F7DA, 0x3F046F69, 0x77FA0A59,
        0x80E4A915, 0x87B08601, 0x9B09E6AD, 0x3B3EE593, 0xE990FD5A, 0x9E34D797,
        0x2CF0B7D9, 0x022B8B51, 0x96D5AC3A, 0x017DA67D, 0xD1CF3ED6, 0x7C7D2D28,
        0x1F9F25CF, 0xADF2B89B, 0x5AD6B472, 0x5A88F54C, 0xE029AC71, 0xE019A5E6,
        0x47B0ACFD, 0xED93FA9B, 0xE8D3C48D, 0x283B57CC, 0xF8D56629, 0x79132E28,
        0x785F0191, 0xED756055, 0xF7960E44, 0xE3D35E8C, 0x15056DD4, 0x88F46DBA,
        0x03A16125, 0x0564F0BD, 0xC3EB9E15, 0x3C9057A2, 0x97271AEC, 0xA93A072A,
        0x1B3F6D9B, 0x1E6321F5, 0xF59C66FB, 0x26DCF319, 0x7533D928, 0xB155FDF5,
        0x03563482, 0x8ABA3CBB, 0x28517711, 0xC20AD9F8, 0xABCC5167, 0xCCAD925F,
        0x4DE81751, 0x3830DC8E, 0x379D5862, 0x9320F991, 0xEA7A90C2, 0xFB3E7BCE,
        0x5121CE64, 0x774FBE32, 0xA8B6E37E, 0xC3293D46, 0x48DE5369, 0x6413E680,
        0xA2AE0810, 0xDD6DB224, 0x69852DFD, 0x09072166, 0xB39A460A, 0x6445C0DD,
        0x586CDECF, 0x1C20C8AE, 0x5BBEF7DD, 0x1B588D40, 0xCCD2017F, 0x6BB4E3BB,
        0xDDA26A7E, 0x3A59FF45, 0x3E350A44, 0xBCB4CDD5, 0x72EACEA8, 0xFA6484BB,
        0x8D6612AE, 0xBF3C6F47, 0xD29BE463, 0x542F5D9E, 0xAEC2771B, 0xF64E6370,
        0x740E0D8D, 0xE75B1357, 0xF8721671, 0xAF537D5D, 0x4040CB08, 0x4EB4E2CC,
        0x34D2466A, 0x0115AF84, 0xE1B00428, 0x95983A1D, 0x06B89FB4, 0xCE6EA048,
        0x6F3F3B82, 0x3520AB82, 0x011A1D4B, 0x277227F8, 0x611560B1, 0xE7933FDC,
        0xBB3A792B, 0x344525BD, 0xA08839E1, 0x51CE794B, 0x2F32C9B7, 0xA01FBAC9,
        0xE01CC87E, 0xBCC7D1F6, 0xCF0111C3, 0xA1E8AAC7, 0x1A908749, 0xD44FBD9A,
        0xD0DADECB, 0xD50ADA38, 0x0339C32A, 0xC6913667, 0x8DF9317C, 0xE0B12B4F,
        0xF79E59B7, 0x43F5BB3A, 0xF2D519FF, 0x27D9459C, 0xBF97222C, 0x15E6FC2A,
        0x0F91FC71, 0x9B941525, 0xFAE59361, 0xCEB69CEB, 0xC2A86459, 0x12BAA8D1,
        0xB6C1075E, 0xE3056A0C, 0x10D25065, 0xCB03A442, 0xE0EC6E0E, 0x1698DB3B,
        0x4C98A0BE, 0x3278E964, 0x9F1F9532, 0xE0D392DF, 0xD3A0342B, 0x8971F21E,
        0x1B0A7441, 0x4BA3348C, 0xC5BE7120, 0xC37632D8, 0xDF359F8D, 0x9B992F2E,
        0xE60B6F47, 0x0FE3F11D, 0xE54CDA54, 0x1EDAD891, 0xCE6279CF, 0xCD3E7E6F,
        0x1618B166, 0xFD2C1D05, 0x848FD2C5, 0xF6FB2299, 0xF523F357, 0xA6327623,
        0x93A83531, 0x56CCCD02, 0xACF08162, 0x5A75EBB5, 0x6E163697, 0x88D273CC,
        0xDE966292, 0x81B949D0, 0x4C50901B, 0x71C65614, 0xE6C6C7BD, 0x327A140A,
        0x45E1D006, 0xC3F27B9A, 0xC9AA53FD, 0x62A80F00, 0xBB25BFE2, 0x35BDD2F6,
        0x71126905, 0xB2040222, 0xB6CBCF7C, 0xCD769C2B, 0x53113EC0, 0x1640E3D3,
        0x38ABBD60, 0x2547ADF0, 0xBA38209C, 0xF746CE76, 0x77AFA1C5, 0x20756060,
        0x85CBFE4E, 0x8AE88DD8, 0x7AAAF9B0, 0x4CF9AA7E, 0x1948C25C, 0x02FB8A8C,
        0x01C36AE4, 0xD6EBE1F9, 0x90D4F869, 0xA65CDEA0, 0x3F09252D, 0xC208E69F,
        0xB74E6132, 0xCE77E25B, 0x578FDFE3, 0x3AC372E6
    }
};

static unsigned int F(x)
unsigned int x;
{
    unsigned int a,b,c,d,y;

    d=x&0xFF;
    x>>=8;
    c=x&0xFF;
    x>>=8;
    b=x&0xFF;
    x>>=8;
    a=x&0xFF;
    y=sbox[0][a]+sbox[1][b];
    y=y^sbox[1][c];
    y=y+sbox[0][d];
    return(y);
}

static void BlowfishEncipher(xl,xr)
unsigned int *xl;
unsigned int *xr;
{
    int i;
    unsigned int Xl,Xr,temp;

    Xl=*xl;
    Xr=*xr;
    for (i=0;i<NUMPBOX;++i) {
        Xl=Xl^pbox[i];
        Xr=F(Xl)^Xr;
        temp=Xl;
        Xl=Xr;
        Xr=temp;
    }
    temp=Xl;
    Xl=Xr;
    Xr=temp;
    Xr=Xr^pbox[NUMPBOX];
    Xl=Xl^pbox[NUMPBOX+1];
    *xl=Xl;
    *xr=Xr;
}

static void BlowfishDecipher(xl,xr)
unsigned int *xl;
unsigned int *xr;
{
    int i;
    unsigned int Xl,Xr,temp;

    Xl=*xl;
    Xr=*xr;
    for (i=NUMPBOX+1;i>1;--i) {
        Xl=Xl^pbox[i];
        Xr=F(Xl)^Xr;
        temp=Xl;
        Xl=Xr;
        Xr=temp;
    }
    temp=Xl;
    Xl=Xr;
    Xr=temp;
    Xr=Xr^pbox[1];
    Xl=Xl^pbox[0];
    *xl=Xl;
    *xr=Xr;
}

static void BlowfishInit(key,keybytes,oldkey)
char *key;
int keybytes;
int oldkey;
{
    int i,j,k,cnt;
    unsigned int data,datal,datar;

    memcpy(pbox,PBOX,sizeof(PBOX));
    memcpy(sbox,SBOX,sizeof(SBOX));
    j=0;
    cnt=0;
    for (i=0;i<NUMPBOX+2;++i) {
        data=0;
        for (k=0;k<4;++k) {
            data=(data<<8)|key[j];
            j++;
            if (j>=keybytes) {
                j=0;
                cnt++;
            }
        }
        pbox[i]=pbox[i]^data;
        if (!oldkey) pbox[i]^=cnt;
    }
    datal=0;
    datar=0;
    for (i=0;i<NUMPBOX+2;i+=2) {
        BlowfishEncipher(&datal,&datar);
        pbox[i]=datal;
        pbox[i+1]=datar;
    }
    for (i=0;i<NUMSBOX;++i) {
        for (j=0;j<256;j+=2) {
            BlowfishEncipher(&datal,&datar);
            sbox[i][j]=datal;
            sbox[i][j+1]=datar;
        }
    }
}

int EncryptString(dest,src,key,bufsize,szenc)
char *dest;
char *src;
char *key;
int  bufsize;
int  szenc;
{
    int i;
    int oldk=0;
    unsigned int l,r;
    unsigned char *s,*d;

    if (szenc) {
        if (*key==SZOLDCRYPT) {
            oldk=1;
            key++;
        }
    }
    else oldk=1;
    BlowfishInit(key,strlen(key),oldk);
    strmcpy(encrbuf,src,bufsize);
    s=encrbuf+strlen(encrbuf);
    for (i=0;i<8;i++) *s++='\0';
    s=encrbuf;
    d=dest;
    if (szenc) {
        strcpy(dest,oldk?SZCRYPTSTROLD:SZCRYPTSTR);
        d+=strlen(dest);
    }
    while (s && *s) {
        l=((*s++)<<24); l|=((*s++)<<16); l|=((*s++)<<8); l|=*s++;
        r=((*s++)<<24); r|=((*s++)<<16); r|=((*s++)<<8); r|=*s++;
        BlowfishEncipher(&l,&r);
        for (i=0;i<6;i++) {
            *d++=base64[r&0x3F];
            r=(r>>6);
        }
        for (i=0;i<6;i++) {
            *d++=base64[l&0x3F];
            l=(l>>6);
        }
    }
    *d=0;
    return(szenc);
}

int Base64Decode(c)
char c;
{
    int i;

    for (i=0;i<64;i++) if (base64[i]==c) return(i);
    return(0);
}

int DecryptString(dest,src,key,bufsize,szenc)
char *dest;
char *src;
char *key;
int  bufsize;
int  szenc;
{
    int i;
    int oldk=0;
    unsigned int l,r;
    unsigned char *s,*d,*x=src;

    if (szenc) {
        if (!(!strncmp(x,SZCRYPTSTR,4) || !strncmp(x,SZCRYPTSTROLD,4))) {
            strmcpy(dest,x,bufsize);
            return(0);
        }
        if (!strncmp(x,SZCRYPTSTROLD,4)) oldk=1;
        x+=strlen(SZCRYPTSTR);
    }
    else oldk=1;
    if (oldk && *key==SZOLDCRYPT) key++;
    BlowfishInit(key,strlen(key),oldk);
    strmcpy(encrbuf,x,bufsize);
    s=encrbuf+strlen(encrbuf);
    for (i=0;i<12;i++) *s++='\0';
    s=x;
    d=dest;
    while (s && *s) {
        l=0;
        r=0;
        for (i=0;i<6;i++) r|=(Base64Decode(*s++))<<(i*6);
        for (i=0;i<6;i++) l|=(Base64Decode(*s++))<<(i*6);
        BlowfishDecipher(&l,&r);
        for (i=0;i<4;i++) *d++=(l & (0xFF<<((3-i)*8)))>>((3-i)*8);
        for (i=0;i<4;i++) *d++=(r & (0xFF<<((3-i)*8)))>>((3-i)*8);
    }
    *d=0;
    return(szenc);
}
