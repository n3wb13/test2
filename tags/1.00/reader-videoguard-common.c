//
// Common videoguard functions.
//
#include "globals.h"
#include "reader-common.h"
#include "reader-videoguard-common.h"

void set_known_card_info(struct s_reader * reader)
{
  /* Set to sensible default values */
  reader->card_baseyear = 1997;
  reader->card_system_version = NDSUNKNOWN;
  reader->card_desc = "VideoGuard Unknown Card";

  NDS_ATR_ENTRY nds_atr_table[]={ // {atr}, atr len, base year, nds version, description
    /* known NDS1 atrs */
    {{ 0x3F, 0x78, 0x13, 0x25, 0x04, 0x40, 0xB0, 0x09, 0x4A, 0x50, 0x01, 0x4E, 0x5A },
       13, 1992, NDS1, "VideoGuard Sky New Zealand (0969)"},
    /* known NDS1+ atrs */
    {{ 0x3F, 0x78, 0x13, 0x25, 0x03, 0x40, 0xB0, 0x20, 0xFF, 0xFF, 0x4A, 0x50, 0x00 },
       13, 1997, NDS12, "VideoGuard DirecTV"},
    /* known NDS2 atrs */
    {{ 0x3F, 0xFD, 0x13, 0x25, 0x02, 0x50, 0x80, 0x0F, 0x33, 0xB0, 0x08, 0xFF, 0xFF, 0x4A, 0x50, 0x90,
       0x00, 0x00, 0x47, 0x4C, 0x01 },
       21, 2004, NDS2, "VideoGuard Sky Brasil GL39 (0907)"},
    {{ 0x3F, 0x7F, 0x11, 0x25, 0x03, 0x33, 0xB0, 0x09, 0x69, 0xFF, 0x4A, 0x50, 0x70, 0x00, 0x00, 0x46,
       0x44, 0x01, 0x00, 0x00 },
       20, 2000, NDS2, "VideoGuard Foxtel Australia (090B)"},
    {{ 0x3F, 0xFF, 0x13, 0x25, 0x03, 0x10, 0x80, 0x33, 0xB0, 0x0E, 0x69, 0xFF, 0x4A, 0x50, 0x70, 0x00,
       0x00, 0x49, 0x54, 0x02, 0x00, 0x00 },
       22, 1997, NDS2, "VideoGuard Sky Italia (0919)"},
    {{ 0x3F, 0xFF, 0x14, 0x25, 0x03, 0x10, 0x80, 0x41, 0xB0, 0x01, 0x69, 0xFF, 0x4A, 0x50, 0x70, 0x00,
       0x00, 0x5A, 0x4A, 0x01, 0x00, 0x00 },
       22, 2004, NDS2, "VideoGuard Dolce Romania (092F)"},
    {{ 0x3F, 0xFF, 0x13, 0x25, 0x03, 0x10, 0x80, 0x54, 0xB0, 0x01, 0x69, 0xFF, 0x4A, 0x50, 0x70, 0x00,
       0x00, 0x41, 0x55, 0x01, 0x00, 0x00 },
       22, 1997, NDS2, "VideoGuard OnoCable Espana (093A)"},
    {{ 0x3F, 0xFD, 0x13, 0x25, 0x02, 0x50, 0x80, 0x0F, 0x33, 0xB0, 0x13, 0x69, 0xFF, 0x4A, 0x50, 0xD0,
       0x80, 0x00, 0x49, 0x54, 0x03 },
       21, 1997, NDS2, "VideoGuard Sky Italia (093B)"},
    {{ 0x3F, 0x7D, 0x11, 0x25, 0x02, 0x41, 0xB0, 0x03, 0x69, 0xFF, 0x4A, 0x50, 0xF0, 0x80, 0x00, 0x56,
       0x54, 0x03 },
       18, 2000, NDS2, "VideoGuard Viasat (093E)"},
    {{ 0x3F, 0xFF, 0x13, 0x25, 0x02, 0x40, 0xB0, 0x12, 0x69, 0xFF, 0x4A, 0x50, 0x90, 0x47, 0x4C, 0x00,
       0x00, 0x00, 0x00, 0x00 },
       20, 2000, NDS2, "VideoGuard Sky Brasil GL23 (0942)"},
    {{ 0x3F, 0xFF, 0x13, 0x25, 0x02, 0x50, 0x80, 0x0F, 0x54, 0xB0, 0x03, 0xFF, 0xFF, 0x4A, 0x50, 0x80,
       0x00, 0x00, 0x00, 0x00, 0x47, 0x4C, 0x05 },
       23, 2009, NDS2, "VideoGuard Sky Brasil GL54 (0943)"},
    {{ 0x3F, 0xFD, 0x13, 0x25, 0x02, 0x50, 0x00, 0x0F, 0x33, 0xB0, 0x0F, 0x69, 0xFF, 0x4A, 0x50, 0xD0,
       0x00, 0x00, 0x53, 0x59, 0x02 },
       21, 1997, NDS2, "VideoGuard BSkyB (0963)"},
    {{ 0x3F, 0xFF, 0x13, 0x25, 0x03, 0x10, 0x80, 0x33, 0xB0, 0x10, 0x69, 0xFF, 0x4A, 0x50, 0x70, 0x00,
       0x00, 0x4E, 0x5A, 0x01, 0x00, 0x00 },
       22, 1992, NDS2, "VideoGuard Sky New Zealand (096A)"},
    {{ 0x3F, 0x7F, 0x13, 0x25, 0x04, 0x33, 0xB0, 0x02, 0x69, 0xFF, 0x4A, 0x50, 0xE0, 0x00, 0x00, 0x54,
       0x42, 0x00, 0x00, 0x00 },
       20, 1997, NDS2, "VideoGuard China (0988)"},
    {{ 0x3F, 0xFF, 0x11, 0x25, 0x03, 0x10, 0x80, 0x41, 0xB0, 0x06, 0x69, 0xFF, 0x4A, 0x50, 0x70, 0x00,
       0x00, 0x41, 0x5A, 0x01, 0x00, 0x11 },
       22, 2006, NDS2, "VideoGuard Astro Malaysia (09AC)"},
    {{ 0x3F, 0xFF, 0x14, 0x25, 0x03, 0x10, 0x80, 0x41, 0xB0, 0x02, 0x69, 0xFF, 0x4A, 0x50, 0x70, 0x80,
       0x00, 0x58, 0x38, 0x01, 0x00, 0x14 },
       22, 1997, NDS2, "VideoGuard TopTV (09B8)"},
    {{ 0x3F, 0xFD, 0x13, 0x25, 0x02, 0x50, 0x80, 0x0F, 0x54, 0xB0, 0x04, 0x69, 0xFF, 0x4A, 0x50, 0xD0,
       0x80, 0x00, 0x49, 0x54, 0x03 },
       21, 1997, NDS2, "VideoGuard Sky Italia (09CD)"},
    {{ 0x3F, 0xFF, 0x13, 0x25, 0x03, 0x10, 0x80, 0x33, 0xB0, 0x11, 0x69, 0xFF, 0x4A, 0x50, 0x50, 0x00,
       0x00, 0x47, 0x54, 0x01, 0x00, 0x00 },
       22, 1997, NDS2, "VideoGuard YES DBS Israel"},
    {{ 0x3F, 0x7F, 0x11, 0x25, 0x03, 0x33, 0xB0, 0x09, 0x69, 0xFF, 0x4A, 0x50, 0x70, 0x00, 0x00, 0x56,
       0x54, 0x01, 0x00, 0x00 },
       20, 2000, NDS2, "VideoGuard Viasat Scandinavia"},
    {{ 0x3F, 0xFF, 0x11, 0x25, 0x03, 0x10, 0x80, 0x41, 0xB0, 0x07, 0x69, 0xFF, 0x4A, 0x50, 0x70, 0x00,
       0x00, 0x50, 0x31, 0x01, 0x00, 0x11 },
       22, 1997, NDS2, "VideoGuard Sky Germany"},
    {{ 0x3F, 0xFF, 0x14, 0x25, 0x03, 0x10, 0x80, 0x41, 0xB0, 0x01, 0x69, 0xFF, 0x4A, 0x50, 0x70, 0x00,
       0x00, 0x5A, 0x48, 0x01, 0x00, 0x00 },
       22, 2004, NDS2, "VideoGuard DSMART Turkey"},
    {{ 0x3F, 0xFF, 0x14, 0x25, 0x03, 0x10, 0x80, 0x54, 0xB0, 0x01, 0x69, 0xFF, 0x4A, 0x50, 0x70, 0x00,
       0x00, 0x4B, 0x57, 0x01, 0x00, 0x00 },
       22, 1997, NDS2, "VideoGuard Kabel BW"},
    {{ 0x3F, 0xFF, 0x14, 0x25, 0x03, 0x10, 0x80, 0x33, 0xB0, 0x10, 0x69, 0xFF, 0x4A, 0x50, 0x70, 0x00,
       0x00, 0x5A, 0x45, 0x01, 0x00, 0x00 },
       22 , 2004, NDS2, "VideoGuard Get Kabel Norway"},
    // NDS Version Unknown as Yet
    {{ 0x3F, 0x7F, 0x13, 0x25, 0x02, 0x40, 0xB0, 0x12, 0x69, 0xFF, 0x4A, 0x50, 0x90, 0x41, 0x55, 0x00,
       0x00, 0x00, 0x00, 0x00 },
       20, 1997, NDSUNKNOWN, "VideoGuard OnoCable Espana (0915)"},
    {{ 0x3F, 0xFF, 0x14, 0x25, 0x03, 0x10, 0x80, 0x41, 0xB0, 0x07, 0x69, 0xFF, 0x4A, 0x50, 0x70, 0x80,
       0x00, 0x58, 0x44, 0x01, 0x00, 0x14 },
       22, 1997, NDSUNKNOWN, "VideoGuard Sky Vivacom (09BD)"}, //45E
    {{ 0 }, 0, 0, 0, NULL}
  };

  int i=0;
  while(nds_atr_table[i].desc) {
    if ((reader->atrlen == nds_atr_table[i].atr_len)
          && (memcmp (reader->atr, nds_atr_table[i].atr, nds_atr_table[i].atr_len) == 0)) {
        reader->card_baseyear=nds_atr_table[i].base_year;
        reader->card_system_version = nds_atr_table[i].nds_version;
        reader->card_desc = nds_atr_table[i].desc;
        break;
    }
    i++;
  }
}

int aes_active=0;

static unsigned short cardkeys[3][32];
static unsigned char stateD3A[16];

static void cCamCryptVG_LongMult(unsigned short *pData, unsigned short *pLen, unsigned int mult, unsigned int carry);
static void cCamCryptVG_PartialMod(unsigned short val, unsigned int count, unsigned short *outkey, const unsigned short *inkey);
static void cCamCryptVG_RotateRightAndHash(unsigned char *p);
static void cCamCryptVG_Reorder16A(unsigned char *dest, const unsigned char *src);
static void cCamCryptVG_ReorderAndEncrypt(unsigned char *p);
static void cCamCryptVG_Process_D0(const unsigned char *ins, unsigned char *data);
static void cCamCryptVG_Process_D1(const unsigned char *ins, unsigned char *data, const unsigned char *status);
static void cCamCryptVG_Decrypt_D3(unsigned char *ins, unsigned char *data, const unsigned char *status);
static void cCamCryptVG_PostProcess_Decrypt(struct s_reader * reader, unsigned char *buff, int len, unsigned char *cw);

struct CmdTab *cmd_table=NULL;

int cw_is_valid(unsigned char *cw, int start)	//returns 1 if cw_is_valid, returns 0 if cw is all zeros
{
  int i;
  for (i = start; i < start+8; i++)
    if (cw[i] != 0) {		//test if cw = 00
      return OK;
    }
  return ERROR;
}

void cAES_SetKey(const unsigned char *key)
{
  AES_set_decrypt_key(key,128,&dkey);
  AES_set_encrypt_key(key,128,&ekey);
  aes_active=1;
}

int cAES_Encrypt(const unsigned char *data, int len, unsigned char *crypt)
{
  if(aes_active) {
    len=(len+15)&(~15); // pad up to a multiple of 16
    int i;
    for(i=0; i<len; i+=16) AES_encrypt(data+i,crypt+i,(const AES_KEY *)&ekey);
    return len;
    }
  return -1;
}


void swap_lb (unsigned char *buff, int len)
{

#if __BYTE_ORDER != __BIG_ENDIAN
  return;

#endif /*  */
  int i;
  unsigned short *tmp;
  for (i = 0; i < len / 2; i++) {
    tmp = (unsigned short *) buff + i;
    *tmp = ((*tmp << 8) & 0xff00) | ((*tmp >> 8) & 0x00ff);
  }
}

inline void __xxor(unsigned char *data, int len, const unsigned char *v1, const unsigned char *v2)
{
  switch(len) { // looks ugly, but the compiler can optimize it very well ;)
    case 16:
      *((unsigned int *)data+3) = *((unsigned int *)v1+3) ^ *((unsigned int *)v2+3);
      *((unsigned int *)data+2) = *((unsigned int *)v1+2) ^ *((unsigned int *)v2+2);
    case 8:
      *((unsigned int *)data+1) = *((unsigned int *)v1+1) ^ *((unsigned int *)v2+1);
    case 4:
      *((unsigned int *)data+0) = *((unsigned int *)v1+0) ^ *((unsigned int *)v2+0);
      break;
    default:
      while(len--) *data++ = *v1++ ^ *v2++;
      break;
    }
}

void cCamCryptVG_SetSeed(unsigned char *Key1, unsigned char *Key2)
{
  swap_lb (Key1, 64);
  swap_lb (Key2, 64);
  memcpy(cardkeys[1],Key1,sizeof(cardkeys[1]));
  memcpy(cardkeys[2],Key2,sizeof(cardkeys[2]));
  swap_lb (Key1, 64);
  swap_lb (Key2, 64);
}

void cCamCryptVG_GetCamKey(unsigned char *buff)
{
  unsigned short *tb2=(unsigned short *)buff, c=1;
  memset(tb2,0,64);
  tb2[0]=1;
  int i;
  for(i=0; i<32; i++) cCamCryptVG_LongMult(tb2,&c,cardkeys[1][i],0);
  swap_lb (buff, 64);
}

static void cCamCryptVG_PostProcess_Decrypt(struct s_reader * reader, unsigned char *rxbuff, int len, unsigned char *cw)
{
  switch(rxbuff[0]) {
    case 0xD0:
      cCamCryptVG_Process_D0(rxbuff,rxbuff+5);
      break;
    case 0xD1:
      cCamCryptVG_Process_D1(rxbuff,rxbuff+5,rxbuff+rxbuff[4]+5);
      break;
    case 0xD3:
      cCamCryptVG_Decrypt_D3(rxbuff,rxbuff+5,rxbuff+rxbuff[4]+5);
      if(rxbuff[1]==0x54) {
        memcpy(cw+0,rxbuff+5,8);
    	memset(cw+8,0,8); //set to 0 so client will know it is invalid unless it is overwritten with a valid cw
        int ind;
        for(ind=15; ind<len+5-10; ind++) {   // +5 for 5 ins bytes, -10 to prevent memcpy ind+3,8 from reading past rxbuffer
                                             // we start searching at 15 because start at 13 goes wrong with 090F 090b and 096a
          if(rxbuff[ind]==0x25) {
            memcpy(cw+8,rxbuff+ind+3,8); //tested on viasat 093E, sky uk 0963, sky it 919  //don't care whether cw is 0 or not
            break;
          }
        }
        // fix for 09ac cards
        // Log decrypted INS54
        // if (rxbuff[1] == 0x54) {
        //   cs_dump (rxbuff, 5, "Decrypted INS54:");
        //   cs_dump (rxbuff + 5, rxbuff[4], "");
        //}
        manage_tag(reader, rxbuff, cw);
      }
      break;
  }
}

static void cCamCryptVG_Process_D0(const unsigned char *ins, unsigned char *data)
{
  switch(ins[1]) {
    case 0xb4:
      swap_lb (data, 64);
      memcpy(cardkeys[0],data,sizeof(cardkeys[0]));
      break;
    case 0xbc:
      {
      swap_lb (data, 64);
      unsigned short *idata=(unsigned short *)data;
      const unsigned short *key1=(const unsigned short *)cardkeys[1];
      unsigned short key2[32];
      memcpy(key2,cardkeys[2],sizeof(key2));
      int count2;
      for(count2=0; count2<32; count2++) {
        unsigned int rem=0, div=key1[count2];
        int i;
        for(i=31; i>=0; i--) {
          unsigned int x=idata[i] | (rem<<16);
          rem=(x%div)&0xffff;
          }
        unsigned int carry=1, t=val_by2on3(div) | 1;
        while(t) {
          if(t&1) carry=((carry*rem)%div)&0xffff;
          rem=((rem*rem)%div)&0xffff;
          t>>=1;
          }
        cCamCryptVG_PartialMod(carry,count2,key2,key1);
        }
      unsigned short idatacount=0;
      int i;
      for(i=31; i>=0; i--) cCamCryptVG_LongMult(idata,&idatacount,key1[i],key2[i]);
      swap_lb (data, 64);
      unsigned char stateD1[16];
      cCamCryptVG_Reorder16A(stateD1,data);
      cAES_SetKey(stateD1);
      break;
      }
  }
}

static void cCamCryptVG_Process_D1(const unsigned char *ins, unsigned char *data, const unsigned char *status)
{
  unsigned char iter[16], tmp[16];
  memset(iter,0,sizeof(iter));
  memcpy(iter,ins,5);
  xor16(iter,stateD3A,iter);
  memcpy(stateD3A,iter,sizeof(iter));

  int datalen=status-data;
  int datalen1=datalen;
  if(datalen<0) datalen1+=15;
  int blocklen=datalen1>>4;
  int i;
  int iblock;
  for(i=0,iblock=0; i<blocklen+2; i++,iblock+=16) {
    unsigned char in[16];
    int docalc=1;
    if(blocklen==i && (docalc=datalen&0xf)) {
      memset(in,0,sizeof(in));
      memcpy(in,&data[iblock],datalen-(datalen1&~0xf));
      }
    else if(blocklen+1==i) {
      memset(in,0,sizeof(in));
      memcpy(&in[5],status,2);
      }
    else
      memcpy(in,&data[iblock],sizeof(in));

    if(docalc) {
      xor16(iter,in,tmp);
      cCamCryptVG_ReorderAndEncrypt(tmp);
      xor16(tmp,stateD3A,iter);
      }
    }
  memcpy(stateD3A,tmp,16);
}

static void cCamCryptVG_Decrypt_D3(unsigned char *ins, unsigned char *data, const unsigned char *status)
{
  if(ins[4]>16) ins[4]-=16;
  if(ins[1]==0xbe) memset(stateD3A,0,sizeof(stateD3A));

  unsigned char tmp[16];
  memset(tmp,0,sizeof(tmp));
  memcpy(tmp,ins,5);
  xor16(tmp,stateD3A,stateD3A);

  int len1=ins[4];
  int blocklen=len1>>4;
  if(ins[1]!=0xbe) blocklen++;

  unsigned char iter[16], states[16][16];
  memset(iter,0,sizeof(iter));
  int blockindex;
  for(blockindex=0; blockindex<blocklen; blockindex++) {
    iter[0]+=blockindex;
    xor16(iter,stateD3A,iter);
    cCamCryptVG_ReorderAndEncrypt(iter);
    xor16(iter,&data[blockindex*16],states[blockindex]);
    if(blockindex==(len1>>4)) {
      int c=len1-(blockindex*16);
      if(c<16) memset(&states[blockindex][c],0,16-c);
      }
    xor16(states[blockindex],stateD3A,stateD3A);
    cCamCryptVG_RotateRightAndHash(stateD3A);
    }
  memset(tmp,0,sizeof(tmp));
  memcpy(tmp+5,status,2);
  xor16(tmp,stateD3A,stateD3A);
  cCamCryptVG_ReorderAndEncrypt(stateD3A);

  memcpy(stateD3A,status-16,sizeof(stateD3A));
  cCamCryptVG_ReorderAndEncrypt(stateD3A);

  memcpy(data,states[0],len1);
  if(ins[1]==0xbe) {
    cCamCryptVG_Reorder16A(tmp,states[0]);
    cAES_SetKey(tmp);
    }
}

static void cCamCryptVG_ReorderAndEncrypt(unsigned char *p)
{
  unsigned char tmp[16];
  cCamCryptVG_Reorder16A(tmp,p);
  cAES_Encrypt(tmp,16,tmp);
  cCamCryptVG_Reorder16A(p,tmp);
}

// reorder AAAABBBBCCCCDDDD to ABCDABCDABCDABCD
static void cCamCryptVG_Reorder16A(unsigned char *dest, const unsigned char *src)
{
  int i;
  int j;
  int k;
  for(i=0,k=0; i<4; i++)
    for(j=i; j<16; j+=4,k++)
      dest[k]=src[j];
}

static void cCamCryptVG_LongMult(unsigned short *pData, unsigned short *pLen, unsigned int mult, unsigned int carry)
{
  int i;
  for(i=0; i<*pLen; i++) {
    carry+=pData[i]*mult;
    pData[i]=(unsigned short)carry;
    carry>>=16;
    }
  if(carry) pData[(*pLen)++]=carry;
}

static void cCamCryptVG_PartialMod(unsigned short val, unsigned int count, unsigned short *outkey, const unsigned short *inkey)
{
  if(count) {
    unsigned int mod=inkey[count];
    unsigned short mult=(inkey[count]-outkey[count-1])&0xffff;
    unsigned int i;
    unsigned int ib1;
    for(i=0,ib1=count-2; i<count-1; i++,ib1--) {
      unsigned int t=(inkey[ib1]*mult)%mod;
      mult=t-outkey[ib1];
      if(mult>t) mult+=mod;
      }
    mult+=val;
    if((val>mult) || (mod<mult)) mult-=mod;
    outkey[count]=(outkey[count]*mult)%mod;
    }
  else
    outkey[0]=val;
}

static void cCamCryptVG_RotateRightAndHash(unsigned char *p)
{
  static const unsigned char table1[256] = {
    0x63,0x7c,0x77,0x7b,0xf2,0x6b,0x6f,0xc5, 0x30,0x01,0x67,0x2b,0xfe,0xd7,0xab,0x76,
    0xca,0x82,0xc9,0x7d,0xfa,0x59,0x47,0xf0, 0xad,0xd4,0xa2,0xaf,0x9c,0xa4,0x72,0xc0,
    0xb7,0xfd,0x93,0x26,0x36,0x3f,0xf7,0xcc, 0x34,0xa5,0xe5,0xf1,0x71,0xd8,0x31,0x15,
    0x04,0xc7,0x23,0xc3,0x18,0x96,0x05,0x9a, 0x07,0x12,0x80,0xe2,0xeb,0x27,0xb2,0x75,
    0x09,0x83,0x2c,0x1a,0x1b,0x6e,0x5a,0xa0, 0x52,0x3b,0xd6,0xb3,0x29,0xe3,0x2f,0x84,
    0x53,0xd1,0x00,0xed,0x20,0xfc,0xb1,0x5b, 0x6a,0xcb,0xbe,0x39,0x4a,0x4c,0x58,0xcf,
    0xd0,0xef,0xaa,0xfb,0x43,0x4d,0x33,0x85, 0x45,0xf9,0x02,0x7f,0x50,0x3c,0x9f,0xa8,
    0x51,0xa3,0x40,0x8f,0x92,0x9d,0x38,0xf5, 0xbc,0xb6,0xda,0x21,0x10,0xff,0xf3,0xd2,
    0xcd,0x0c,0x13,0xec,0x5f,0x97,0x44,0x17, 0xc4,0xa7,0x7e,0x3d,0x64,0x5d,0x19,0x73,
    0x60,0x81,0x4f,0xdc,0x22,0x2a,0x90,0x88, 0x46,0xee,0xb8,0x14,0xde,0x5e,0x0b,0xdb,
    0xe0,0x32,0x3a,0x0a,0x49,0x06,0x24,0x5c, 0xc2,0xd3,0xac,0x62,0x91,0x95,0xe4,0x79,
    0xe7,0xc8,0x37,0x6d,0x8d,0xd5,0x4e,0xa9, 0x6c,0x56,0xf4,0xea,0x65,0x7a,0xae,0x08,
    0xba,0x78,0x25,0x2e,0x1c,0xa6,0xb4,0xc6, 0xe8,0xdd,0x74,0x1f,0x4b,0xbd,0x8b,0x8a,
    0x70,0x3e,0xb5,0x66,0x48,0x03,0xf6,0x0e, 0x61,0x35,0x57,0xb9,0x86,0xc1,0x1d,0x9e,
    0xe1,0xf8,0x98,0x11,0x69,0xd9,0x8e,0x94, 0x9b,0x1e,0x87,0xe9,0xce,0x55,0x28,0xdf,
    0x8c,0xa1,0x89,0x0d,0xbf,0xe6,0x42,0x68, 0x41,0x99,0x2d,0x0f,0xb0,0x54,0xbb,0x16,
    };
  unsigned char t1=p[15];
  int i;
  for(i=0; i<16; i++) {
    unsigned char t2=t1;
    t1=p[i]; p[i]=table1[(t1>>1)|((t2&1)<<7)];
    }
}

int status_ok(const unsigned char *status)
{
    //cs_log("[videoguard-reader] check status %02x%02x", status[0],status[1]);
    return (status[0] == 0x90 || status[0] == 0x91)
           && (status[1] == 0x00 || status[1] == 0x01
               || status[1] == 0x20 || status[1] == 0x21
               || status[1] == 0x80 || status[1] == 0x81
               || status[1] == 0xa0 || status[1] == 0xa1);
}

void memorize_cmd_table (const unsigned char *mem, int size){
  cmd_table=(struct CmdTab *)malloc(sizeof(unsigned char) * size);
  memcpy(cmd_table,mem,size);
}

void manage_tag(struct s_reader * reader, unsigned char *rxbuff, unsigned char *cw)
{
  unsigned char tag,len,len2;
  bool valid_0x55=FALSE;
  unsigned char *body;
  unsigned char buffer[0x10];
  int a=0x13;
  len2=rxbuff[4];
  while(a<len2+5-9)  //  +5 for 5 ins bytes, -9 (body=8 len=1) to prevent memcpy(buffer+8,body,8) from reading past rxbuff
  {
    tag=rxbuff[a];
    len=rxbuff[a+1];
    body=rxbuff+a+2;
    switch(tag)
    {
      case 0x55:{
        if(body[0]==0x84){	//Tag 0x56 has valid data...
          valid_0x55=TRUE;
        }
      }break;
      case 0x56:{
        memcpy(buffer+8,body,8);
      }break;
    }
    a+=len+2;
  }
  if(valid_0x55){
    memcpy(buffer,rxbuff+5,8);
    if(aes_decrypt_from_list(reader->aes_list,reader->caid[0], 0, AESKEY_ASTRO,buffer, 16)) {
      memcpy(cw+0,buffer,8);	// copy calculated CW in right place
    }
  }
}

int cmd_table_get_info(const unsigned char *cmd, unsigned char *rlen, unsigned char *rmode)
{
  struct CmdTabEntry *pcte=cmd_table->e;
  int i;
  for(i=0; i<cmd_table->Nentries; i++,pcte++)
    if(cmd[1]==pcte->cmd) {
      *rlen=pcte->len;
      *rmode=pcte->mode;
      return 1;
      }
  return 0;
}

int cmd_exists(const unsigned char *cmd)
{
  struct CmdTabEntry *pcte=cmd_table->e;
  int i;
  for(i=0; i<cmd_table->Nentries; i++,pcte++)
    if(cmd[1]==pcte->cmd) {
      return 1;
      }
  return 0;
}

int read_cmd_len(struct s_reader * reader, const unsigned char *cmd)
{
  def_resp;
  unsigned char cmd2[5];
  memcpy(cmd2,cmd,5);
  cmd2[3]=0x80;
  cmd2[4]=1;
  // some card reply with L 91 00 (L being the command length).

  if(!write_cmd_vg(cmd2,NULL) || !status_ok(cta_res+1)) {
    cs_debug("[videoguard-reader] failed to read %02x%02x cmd length (%02x %02x)",cmd[1],cmd[2],cta_res[1],cta_res[2]);
    return -1;
    }
  return cta_res[0];
}

int do_cmd(struct s_reader * reader, const unsigned char *ins, const unsigned char *txbuff, unsigned char *rxbuff,
           unsigned char *cw, unsigned char * cta_res)
{
  ushort cta_lr;
  unsigned char ins2[5];
  memcpy(ins2,ins,5);
  unsigned char len=0, mode=0;
  if(cmd_table_get_info(ins2,&len,&mode)) {
    if(len==0xFF && mode==2) {
      if(ins2[4]==0) ins2[4]=len=read_cmd_len(reader, ins2);
      }
    else if(mode!=0) ins2[4]=len;
    }
  if(ins2[0]==0xd3) ins2[4]=len+16;
  len=ins2[4];

  unsigned char tmp[264];
  if(!rxbuff) rxbuff=tmp;
  if(mode>1) {
    if(!write_cmd_vg(ins2,NULL) || !status_ok(cta_res+len)) return -1;
    memcpy(rxbuff,ins2,5);
    memcpy(rxbuff+5,cta_res,len);
    memcpy(rxbuff+5+len,cta_res+len,2);
    }
  else {
    if(!write_cmd_vg(ins2,(uchar *)txbuff) || !status_ok(cta_res)) return -2;
    memcpy(rxbuff,ins2,5);
    memcpy(rxbuff+5,txbuff,len);
    memcpy(rxbuff+5+len,cta_res,2);
    }

  cCamCryptVG_PostProcess_Decrypt(reader, rxbuff,len,cw);

  // manage_tag(reader, rxbuff, cw); moved into cCamCryptVG_PostProcess_Decrypt as only needed on classD3 ins54

  return len;
}

void rev_date_calc(const unsigned char *Date, int *year, int *mon, int *day, int *hh, int *mm, int *ss, int base_year)
{
  *year=(Date[0]/12)+base_year;
  *mon=(Date[0]%12)+1;
  *day=Date[1] & 0x1f;
  *hh=Date[2]/8;
  *mm=(0x100*(Date[2]-*hh*8)+Date[3])/32;
  *ss=(Date[3]-*mm*32)*2;
}

void do_post_dw_hash(unsigned char *cw, unsigned char *ecm_header_data)
{
  int i, ecmi, ecm_header_count;
  unsigned char buffer[0x80];
  unsigned char md5_digest[0x10];
  const unsigned short Hash3[] = {0x0123,0x4567,0x89AB,0xCDEF,0xF861,0xCB52};
  const unsigned char Hash4[] = {0x0B,0x04,0x07,0x08,0x05,0x09,0x0B,0x0A,0x07,0x02,0x0A,0x05,0x04,0x08,0x0D,0x0F};
  const unsigned short NdTabB001[0x15][0x20] = {
    {0xEAF1, 0x0237, 0x29D0, 0xBAD2, 0xE9D3, 0x8BAE, 0x2D6D, 0xCD1B,
     0x538D, 0xDE6B, 0xA634, 0xF81A, 0x18B5, 0x5087, 0x14EA, 0x672E,
     0xF0FC, 0x055E, 0x62E5, 0xB78F, 0x5D09, 0x0003, 0xE4E8, 0x2DCE,
     0x6BE0, 0xAC4E, 0xF485, 0x6967, 0xF28C, 0x97A0, 0x01EF, 0x0100},
    {0xC539, 0xF5B9, 0x9099, 0x013A, 0xD4B9, 0x6AB5, 0xEA67, 0x7EB4,
     0x6C30, 0x4BF0, 0xB810, 0xB0B5, 0xB76D, 0xA751, 0x1AE7, 0x14CA,
     0x4F4F, 0x1586, 0x2608, 0x10B1, 0xE7E1, 0x48BE, 0x7DDD, 0x5ECB,
     0xCFBF, 0x323B, 0x8B31, 0xB131, 0x0F1A, 0x664B, 0x0140, 0x0100},
    {0x3C7D, 0xBDC4, 0xFEC7, 0x26A6, 0xB0A0, 0x6E55, 0xF710, 0xF9BF,
     0x0023, 0xE81F, 0x41CA, 0xBE32, 0xB461, 0xE92D, 0xF1AF, 0x409F,
     0xFC85, 0xFE5B, 0x7FCE, 0x17F5, 0x01AB, 0x4A46, 0xEB05, 0xA251,
     0xDC6F, 0xF0C0, 0x10F0, 0x1D51, 0xEFAA, 0xE9BF, 0x0100, 0x0100},
    {0x1819, 0x0CAA, 0x9067, 0x607A, 0x7576, 0x1CBC, 0xE51D, 0xBF77,
     0x7EC6, 0x839E, 0xB695, 0xF096, 0xDC10, 0xCB69, 0x4654, 0x8E68,
     0xD62D, 0x4F1A, 0x4227, 0x92AC, 0x9064, 0x6BD1, 0x1E75, 0x2747,
     0x00DA, 0xA6A6, 0x6CF1, 0xD151, 0xBE56, 0x3E33, 0x0128, 0x0100},
    {0x4091, 0x09ED, 0xD494, 0x6054, 0x1869, 0x71D5, 0xB572, 0x7BF1,
     0xE925, 0xEE2D, 0xEEDE, 0xA13C, 0x6613, 0x9BAB, 0x122D, 0x7AE4,
     0x5268, 0xE6C9, 0x50CB, 0x79A1, 0xF212, 0xA062, 0x6B48, 0x70B3,
     0xF6B0, 0x06D5, 0xF8AB, 0xECF5, 0x6255, 0xEDD8, 0x79D2, 0x290A},
    {0xD3CF, 0x014E, 0xACB3, 0x8F6B, 0x0F2C, 0xA5D8, 0xE8E0, 0x863D,
     0x80D5, 0x5705, 0x658A, 0x8BC2, 0xEE46, 0xD3AE, 0x0199, 0x0100,
     0x4A35, 0xABE4, 0xF976, 0x935A, 0xA8A5, 0xBAE9, 0x24D0, 0x71AA,
     0xB3FE, 0x095E, 0xAB06, 0x4CD5, 0x2F0D, 0x1ACB, 0x59F3, 0x4C50},
    {0xFD27, 0x0F8E, 0x191A, 0xEEE7, 0x2F49, 0x3A05, 0x3267, 0x4F88,
     0x38AE, 0xFCE9, 0x9476, 0x18C6, 0xF961, 0x4EF0, 0x39D0, 0x42E6,
     0xB747, 0xE625, 0xB68E, 0x5100, 0xF92A, 0x86FE, 0xE79B, 0xEE91,
     0x21D5, 0x4C3C, 0x683D, 0x5AD1, 0x1B49, 0xF407, 0x0194, 0x0100},
    {0x4BF9, 0xDC0D, 0x9478, 0x5174, 0xCB4A, 0x8A89, 0x4D6A, 0xFED8,
     0xF123, 0xA8CD, 0xEEE7, 0xA6D1, 0xB763, 0xF5E2, 0xE085, 0x01EF,
     0xE466, 0x9FA3, 0x2F68, 0x2190, 0x423F, 0x287F, 0x7F3F, 0x09F6,
     0x2111, 0xA963, 0xD0BB, 0x674A, 0xBA72, 0x45F9, 0xF186, 0xB8F5},
    {0x0010, 0xD1B9, 0xB164, 0x9E87, 0x1F49, 0x6950, 0x2DBF, 0x38D3,
     0x2EB0, 0x3E8E, 0x91E6, 0xF688, 0x7E41, 0x566E, 0x01B0, 0x0100,
     0x24A1, 0x73D8, 0xA0C3, 0xF71B, 0xA0A5, 0x2A06, 0xBA46, 0xFEC3,
     0xDD4C, 0x52CC, 0xF9BC, 0x3B7E, 0x3812, 0x0666, 0xB74B, 0x40F8},
    {0x28F2, 0x7C81, 0xFC92, 0x6FBD, 0x53D6, 0x72A3, 0xBBDF, 0xB6FC,
     0x9CE5, 0x2331, 0xD4F6, 0xC5BB, 0xE8BB, 0x6676, 0x02D9, 0x2F0E,
     0xD009, 0xD136, 0xCD09, 0x7551, 0x1826, 0x9D9B, 0x63EA, 0xFC63,
     0x68CD, 0x3672, 0xCB95, 0xD28E, 0xF1CD, 0x20CA, 0x014C, 0x0100},
    {0xE539, 0x55B7, 0x989D, 0x21C4, 0x463A, 0xE68F, 0xF8B5, 0xE5C5,
     0x662B, 0x35BF, 0x3C50, 0x0131, 0xF4BF, 0x38B2, 0x41BC, 0xB829,
     0x02B7, 0x6B8F, 0xA25C, 0xAFD2, 0xD84A, 0x2243, 0x53EB, 0xC6C9,
     0x2E14, 0x181F, 0x8F96, 0xDF0E, 0x0D4C, 0x30F6, 0xFFE1, 0x9DDA},
    {0x30B6, 0x777E, 0xDA3D, 0xAF77, 0x205E, 0xC90B, 0x856B, 0xB451,
     0x3BCC, 0x76C2, 0x8ACF, 0xDCB1, 0xA5E5, 0xDD64, 0x0197, 0x0100,
     0xE751, 0xB661, 0x0404, 0xDB4A, 0xE9DD, 0xA400, 0xAF26, 0x3F5E,
     0x904B, 0xA924, 0x09E0, 0xE72B, 0x825B, 0x2C50, 0x6FD0, 0x0D52},
    {0x2730, 0xC2BA, 0x9E44, 0x5815, 0xFC47, 0xB21D, 0x67B8, 0xF8B9,
     0x047D, 0xB0AF, 0x9F14, 0x741B, 0x4668, 0xBE54, 0xDE16, 0xDB14,
     0x7CB7, 0xF2B8, 0x0683, 0x762C, 0x09A0, 0x9507, 0x7F92, 0x022C,
     0xBA6A, 0x7D52, 0x0AF4, 0x1BC3, 0xB46A, 0xC4FD, 0x01C2, 0x0100},
    {0x7611, 0x66F3, 0xEE87, 0xEDD3, 0xC559, 0xEFD4, 0xDC59, 0xF86B,
     0x6D1C, 0x1C85, 0x9BB1, 0x3373, 0x763F, 0x4EBE, 0x1BF3, 0x99B5,
     0xD721, 0x978F, 0xCF5C, 0xAC51, 0x0984, 0x7462, 0x8F0C, 0x2817,
     0x4AD9, 0xFD41, 0x6678, 0x7C85, 0xD330, 0xC9F8, 0x1D9A, 0xC622},
    {0x5AE4, 0xE16A, 0x60F6, 0xFD45, 0x668C, 0x29D6, 0x0285, 0x6B92,
     0x92C2, 0x21DE, 0x45E0, 0xEF3D, 0x8B0D, 0x02CD, 0x0198, 0x0100,
     0x9E6D, 0x4D38, 0xDEF9, 0xE6F2, 0xF72E, 0xB313, 0x14F2, 0x390A,
     0x2D67, 0xC71E, 0xCB69, 0x7F66, 0xD3CF, 0x7F8A, 0x81D9, 0x9DDE},
    {0x85E3, 0x8F29, 0x36EB, 0xC968, 0x3696, 0x59F6, 0x7832, 0xA78B,
     0xA1D8, 0xF5CF, 0xAB64, 0x646D, 0x7A2A, 0xBAF8, 0xAA87, 0x41C7,
     0x5120, 0xDE78, 0x738D, 0xDC1A, 0x268D, 0x5DF8, 0xED69, 0x1C8A,
     0xBC85, 0x3DCD, 0xAE30, 0x0F8D, 0xEC89, 0x3ABD, 0x0166, 0x0100},
    {0xB8BD, 0x643B, 0x748E, 0xBD63, 0xEC6F, 0xE23A, 0x9493, 0xDD76,
     0x0A62, 0x774F, 0xCD68, 0xA67A, 0x9A23, 0xC8A8, 0xBDE5, 0x9D1B,
     0x2B86, 0x8B36, 0x5428, 0x1DFB, 0xCD1D, 0x0713, 0x29C2, 0x8E8E,
     0x5207, 0xA13F, 0x6005, 0x4F5E, 0x52E0, 0xE7C8, 0x6D1C, 0x3E34},
    {0x581D, 0x2BFA, 0x5E1D, 0xA891, 0x1069, 0x1DA4, 0x39A0, 0xBE45,
     0x5B9A, 0x7333, 0x6F3E, 0x8637, 0xA550, 0xC9E9, 0x5C6C, 0x42BA,
     0xA712, 0xC3EA, 0x3808, 0x0910, 0xAA4D, 0x5B25, 0xABCD, 0xE680,
     0x96AD, 0x2CEC, 0x8EBB, 0xA47D, 0x1690, 0xE8FB, 0x01C8, 0x0100},
    {0x73B9, 0x82BC, 0x9EBC, 0xB130, 0x0DA5, 0x8617, 0x9F7B, 0x9766,
     0x205D, 0x752D, 0xB05C, 0x2A17, 0xA75C, 0x18EF, 0x8339, 0xFD34,
     0x8DA2, 0x7970, 0xD0B4, 0x70F1, 0x3765, 0x7380, 0x7CAF, 0x570E,
     0x6440, 0xBC44, 0x0743, 0x2D02, 0x0419, 0xA240, 0x2113, 0x1AD4},
    {0x1EB5, 0xBBFF, 0x39B1, 0x3209, 0x705F, 0x15F4, 0xD7AD, 0x340B,
     0xC2A6, 0x25CA, 0xF412, 0x9570, 0x0F4F, 0xE4D5, 0x1614, 0xE464,
     0x911A, 0x0F0E, 0x07DA, 0xA929, 0x2379, 0xD988, 0x0AA6, 0x3B57,
     0xBF63, 0x71FB, 0x72D5, 0x26CE, 0xB0AF, 0xCF45, 0x011B, 0x0100},
    {0x9999, 0x98FE, 0xA108, 0x6588, 0xF90B, 0x4554, 0xFF38, 0x4642,
     0x8F5F, 0x6CC3, 0x4E8E, 0xFF7E, 0x64C2, 0x50CA, 0x0E7F, 0xAD7D,
     0x6AAB, 0x33C1, 0xE1F4, 0x6165, 0x7894, 0x83B9, 0x0A0C, 0x38AF,
     0x5803, 0x18C0, 0xFA36, 0x592C, 0x4548, 0xABB8, 0x1527, 0xAEE9}
  };


  //ecm_header_data = 01 03 b0 01 01
  if (!cw_is_valid(cw,0))         //if cw is all zero, keep it that way
  {
    return;
  }
  ecm_header_count = ecm_header_data[0];
  for (i = 0, ecmi = 1; i < ecm_header_count; i++)
  {
    if (ecm_header_data[ecmi + 1] != 0xb0)
    {
      ecmi += ecm_header_data[ecmi] + 1;
    }
    else
    {
      switch (ecm_header_data[ecmi + 2])
      {                         //b0 01
      case 1:
        {
          unsigned short hk[8], i, j, m = 0;
          for (i = 0; i < 6; i++)
            hk[2 + i] = Hash3[i];
          for (i = 0; i < 2; i++)
          {
            for (j = 0; j < 0x48; j += 2)
            {
              if (i)
              {
                hk[0] = ((hk[3] & hk[5]) | ((~hk[5]) & hk[4]));
              }
              else
              {
                hk[0] = ((hk[3] & hk[4]) | ((~hk[3]) & hk[5]));
              }
              if (j < 8)
              {
                hk[0] = (hk[0] + ((cw[j + 1] << 8) | cw[j]));
              }
              if (j == 8)
              {
                hk[0] = (hk[0] + 0x80);
              }
              hk[0] = (hk[0] + hk[2] + (0xFF & NdTabB001[ecm_header_data[ecmi + 3]][m >> 1] >> ((m & 1) << 3)));
              hk[1] = hk[2];
              hk[2] = hk[3];
              hk[3] = hk[4];
              hk[4] = hk[5];
              hk[5] = hk[6];
              hk[6] = hk[7];
              hk[7] = hk[2] + (((hk[0] << Hash4[m & 0xF]) | (hk[0] >> (0x10 - Hash4[m & 0xF]))));
              m = (m + 1) & 0x3F;
            }
          }
          for (i = 0; i < 6; i++)
          {
            hk[2 + i] += Hash3[i];
          }
          for (i = 0; i < 7; i++)
          {
            cw[i] = hk[2 + (i >> 1)] >> ((i & 1) << 3);
          }
          cw[3] = (cw[0] + cw[1] + cw[2]) & 0xFF;
          cw[7] = (cw[4] + cw[5] + cw[6]) & 0xFF;
          cs_ddump(cw, 8, "Postprocessed Case 1 DW:");
          break;
        }
      case 3:
        {
          memset(buffer, 0, sizeof(buffer));
          memcpy(buffer, cw, 8);
          memcpy(buffer + 8, &ecm_header_data[ecmi + 3], ecm_header_data[ecmi] - 2);
          MD5(buffer, 8 + ecm_header_data[ecmi] - 2, md5_digest);
          memcpy(cw, md5_digest, 8);
          cs_ddump(cw, 8, "Postprocessed Case 3 DW:");
          break;
        }
      case 2:
        {
          /* Method 2 left out */
          //memcpy(DW_OUTPUT, DW_INPUT, 8);
          break;
        }
      }
    }
  }
}
