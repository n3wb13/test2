#include "globals.h"
#include "reader-common.h"

#define CMD_LEN 5

static void RotateBytes1(unsigned char *out, unsigned char *in, int32_t n)
{
  // loop is executed atleast once, so it's not a good idea to
  // call with n=0 !!
  out+=n;
  do { *(--out)=*(in++); } while(--n);
}

static void RotateBytes2(unsigned char *in, int32_t n)
{
  // loop is executed atleast once, so it's not a good idea to
  // call with n=0 !!
  unsigned char *e=in+n-1;
  do
  {
    unsigned char temp=*in;
    *in++=*e;
    *e-- =temp;
  } while(in<e);
}

static int32_t Input(BIGNUM *d, unsigned char *in, int32_t n, int32_t LE)
{
  if (LE)
  {
    unsigned char tmp[n];
    RotateBytes1(tmp,in,n);
    return(BN_bin2bn(tmp,n,d)!=0);
  }
  else
    return(BN_bin2bn(in,n,d)!=0);
}

static int32_t Output(unsigned char *out, int32_t n, BIGNUM *r, int32_t LE)
{
  int32_t s=BN_num_bytes(r);
  if (s>n)
  {
    unsigned char buff[s];
    cs_debug_mask(D_READER, "[cryptoworks-reader] rsa: RSA len %d > %d, truncating", s, n);
    BN_bn2bin(r,buff);
    memcpy(out,buff+s-n,n);
  }
  else if (s<n)
  {
    int32_t l=n-s;
    cs_debug_mask(D_READER, "[cryptoworks-reader] rsa: RSA len %d < %d, padding", s, n);
    memset(out,0,l);
    BN_bn2bin(r,out+l);
  }
  else
    BN_bn2bin(r,out);
  if (LE)
    RotateBytes2(out,n);
  return(s);
}

static int32_t cw_RSA(unsigned char *out, unsigned char *in, int32_t n, BIGNUM *exp, BIGNUM *mod, int32_t LE)
{
  int32_t rc=0;
  BN_CTX *ctx;
  BIGNUM *r, *d;
  ctx=BN_CTX_new();
  r=BN_new();
  d=BN_new();
  if (Input(d,in,n,LE))
  {
    if(BN_mod_exp(r,d,exp,mod,ctx))
      rc=Output(out,n,r,LE);
    else
      cs_log("[cryptoworks-reader] rsa: mod-exp failed");
  }
  BN_CTX_free(ctx);
  BN_free(d);
  BN_free(r);
  return(rc);
}

static char *chid_date(uchar *ptr, char *buf, int32_t l)
{
  if (buf)
  {
    snprintf(buf, l, "%04d/%02d/%02d",
                     1990+(ptr[0]>>1), ((ptr[0]&1)<<3)|(ptr[1]>>5), ptr[1]&0x1f);
  }
  return(buf);
}

static int32_t select_file(struct s_reader * reader, uchar f1, uchar f2, uchar * cta_res, uint16_t * p_cta_lr)
{
  uint16_t cta_lr;
  uchar insA4[] = {0xA4, 0xA4, 0x00, 0x00, 0x02, 0x00, 0x00};
  insA4[5]=f1;
  insA4[6]=f2;
  write_cmd(insA4, insA4+5);	// select file
  *p_cta_lr = cta_lr;
  return((cta_res[0]==0x9f)&&(cta_res[1]==0x11));
}

static int32_t read_record(struct s_reader * reader, uchar rec, uchar * cta_res)
{
  uint16_t cta_lr;
  uchar insA2[] = {0xA4, 0xA2, 0x00, 0x00, 0x01, 0x00};
  uchar insB2[] = {0xA4, 0xB2, 0x00, 0x00, 0x00};

  insA2[5]=rec;
  write_cmd(insA2, insA2+5);	// select record
  if (cta_res[0]!=0x9f)
    return(-1);
  insB2[4]=cta_res[1];		// get len
  write_cmd(insB2, NULL);	// read record
  if ((cta_res[cta_lr-2]!=0x90) || (cta_res[cta_lr-1]))
    return(-1);
  return(cta_lr-2);
}

/*
int32_t cryptoworks_send_pin(struct s_reader * reader)
{
  unsigned char insPIN[] = { 0xA4, 0x20, 0x00, 0x00, 0x04, 0x00,0x00,0x00,0x00 }; //Verify PIN  
  
  if(reader->pincode[0] && (reader->pincode[0]&0xF0)==0x30)
  {
	  memcpy(insPIN+5,reader->pincode,4);
	
	  write_cmd(insPIN, insPIN+5);
	  cs_ri_log (reader, "sending pincode to card");  
	  if((cta_res[0]==0x98)&&(cta_res[1]==0x04)) cs_ri_log (reader, "bad pincode");
	  	 
	  return OK;
  }
  
  return(0);
}
*/

static int32_t cryptoworks_disable_pin(struct s_reader * reader)
{
  def_resp;
  unsigned char insPIN[] = { 0xA4, 0x26, 0x00, 0x00, 0x04, 0x00,0x00,0x00,0x00 }; //disable PIN  
  
  if(reader->pincode[0] && (reader->pincode[0]&0xF0)==0x30)
  {
	  memcpy(insPIN+5,reader->pincode,4);
	
	  write_cmd(insPIN, insPIN+5);
	  cs_ri_log (reader, "disable pincode to card");
	  if((cta_res[0]==0x98)&&(cta_res[1]==0x04)) cs_ri_log (reader, "bad pincode");
	  return ERROR;
  }
  return OK;
}

static int32_t cryptoworks_card_init(struct s_reader * reader, ATR newatr)
{
  get_atr;
  def_resp;
  int32_t i;
  uint32_t mfid=0x3F20;
  static const uchar cwexp[] = { 1, 0 , 1};
  uchar insA4C[]= {0xA4, 0xC0, 0x00, 0x00, 0x11};
  uchar insB8[] = {0xA4, 0xB8, 0x00, 0x00, 0x0c};
  uchar issuerid=0;
  char issuer[20]={0};
  char *unknown="unknown", *pin=unknown, ptxt[CS_MAXPROV<<2]={0};

  if ((atr[6]!=0xC4) || (atr[9]!=0x8F) || (atr[10]!=0xF1)) return ERROR;

  cs_log("[cryptoworks-reader] card detected");
  cs_log("[cryptoworks-reader] type: CryptoWorks");

  reader->caid=0xD00;
  reader->nprov=0;
  reader->ucpk_valid = 0;
  memset(reader->prid, 0, sizeof(reader->prid));

  write_cmd(insA4C, NULL);		// read masterfile-ID
  if ((cta_res[0]==0xDF) && (cta_res[1]>=6))
    mfid=(cta_res[6]<<8)|cta_res[7];

  select_file(reader, 0x3f, 0x20, cta_res, &cta_lr);
  insB8[2]=insB8[3]=0;		// first
  for(cta_res[0]=0xdf; cta_res[0]==0xdf;)
  {
    write_cmd(insB8, NULL);		// read provider id's
    if (cta_res[0]!=0xdf) break;
    if (((cta_res[4]&0x1f)==0x1f) && (reader->nprov<CS_MAXPROV))
    {
      snprintf(ptxt+strlen(ptxt), sizeof(ptxt)-strlen(ptxt), ",%02X", cta_res[5]);
      reader->prid[reader->nprov++][3]=cta_res[5];
    }
    insB8[2]=insB8[3]=0xff;	// next
  }
  for (i=reader->nprov; i<CS_MAXPROV; i++)
    memset(&reader->prid[i][0], 0xff, 4);

  select_file(reader, 0x2f, 0x01, cta_res, &cta_lr);		// read caid
  if (read_record(reader, 0xD1, cta_res)>=4)
    reader->caid=(cta_res[2]<<8)|cta_res[3];

  if (read_record(reader, 0x80, cta_res)>=7)		// read serial
    memcpy(reader->hexserial, cta_res+2, 5);
  cs_ri_log (reader, "type: CryptoWorks, caid: %04X, ascii serial: %llu, hex serial: %s",
            reader->caid, b2ll(5, reader->hexserial),cs_hexdump(0, reader->hexserial, 5));

  if (read_record(reader, 0x9E, cta_res)>=66)	// read ISK
  {
    uchar keybuf[256];
    BIGNUM *ipk;
    if (search_boxkey(reader->caid, (char *)keybuf))
    {
      ipk=BN_new();
      BN_bin2bn(cwexp, sizeof(cwexp), &reader->exp);
      BN_bin2bn(keybuf, 64, ipk);
      cw_RSA(cta_res+2, cta_res+2, 0x40, &reader->exp, ipk, 0);
      BN_free(ipk);
      reader->ucpk_valid =(cta_res[2]==((mfid & 0xFF)>>1));
      if (reader->ucpk_valid)
      {
        cta_res[2]|=0x80;
        BN_bin2bn(cta_res+2, 0x40, &reader->ucpk);
        cs_ddump_mask(D_READER, cta_res+2, 0x40, "IPK available -> session-key:");
      }
      else
      {
        reader->ucpk_valid =(keybuf[0]==(((mfid & 0xFF)>>1)|0x80));
        if (reader->ucpk_valid)
        {
          BN_bin2bn(keybuf, 0x40, &reader->ucpk);
          cs_ddump_mask(D_READER, keybuf, 0x40, "session-key found:");
        }
        else
          cs_log("[cryptoworks-reader] invalid IPK or session-key for CAID %04X !", reader->caid);
      }
    }
  }
  if (read_record(reader, 0x9F, cta_res)>=3)
    issuerid=cta_res[2];
  if (read_record(reader, 0xC0, cta_res)>=16)
  {
    cs_strncpy(issuer, (const char *)cta_res+2, sizeof(issuer));
    trim(issuer);
  }
  else
    cs_strncpy(issuer, unknown, sizeof(issuer));

  select_file(reader, 0x3f, 0x20, cta_res, &cta_lr);
  select_file(reader, 0x2f, 0x11, cta_res, &cta_lr);		// read pin
  if (read_record(reader, atr[8], cta_res)>=7)
  {
    cta_res[6]=0;
    pin=(char *)cta_res+2;
  }
  cs_ri_log (reader, "issuer: %s, id: %02X, bios: v%d, pin: %s, mfid: %04X", issuer, issuerid, atr[7], pin, mfid);
  cs_ri_log (reader, "providers: %d (%s)", reader->nprov, ptxt+1);
  
  cryptoworks_disable_pin(reader);
  	
  return OK;
}

static int32_t cryptoworks_do_ecm(struct s_reader * reader, ECM_REQUEST *er)
{
  def_resp;
	int32_t r=0;
  unsigned char ins4C[] = { 0xA4,0x4C,0x00,0x00,0x00 };
  unsigned char insC0[] = { 0xA4,0xC0,0x00,0x00,0x1C };
  unsigned char nanoD4[10];
  int32_t secLen=check_sct_len(er->ecm,-5+(reader->ucpk_valid ? sizeof(nanoD4):0));

  if(secLen>5)
  {
    int32_t i;
    uchar *ecm=er->ecm;
    uchar buff[MAX_LEN];

    if(reader->ucpk_valid)
    {
      memcpy(buff,er->ecm,secLen);
      nanoD4[0]=0xD4;
      nanoD4[1]=0x08;
      for (i=2; i<(int)sizeof(nanoD4); i++)
        nanoD4[i]=rand();
      memcpy(&buff[secLen], nanoD4, sizeof(nanoD4));
      ecm=buff;
      secLen+=sizeof(nanoD4);
    }

    ins4C[3]=reader->ucpk_valid ? 2 : 0;
    ins4C[4]=secLen-5;
    write_cmd(ins4C, ecm+5);
    if (cta_res[cta_lr-2]==0x9f)
    {
      insC0[4]=cta_res[cta_lr-1];
      write_cmd(insC0, NULL);
      for(i=0; i<secLen && r<2; )
      {
        int32_t n=cta_res[i+1];
        switch(cta_res[i])
	{
          case 0x80:
            cs_debug_mask(D_READER, "[cryptoworks-reader] nano 80 (serial)");
            break;
          case 0xD4:
            cs_debug_mask(D_READER, "[cryptoworks-reader] nano D4 (rand)");
            if(n<8 || memcmp(&cta_res[i],nanoD4,sizeof(nanoD4))){
              cs_debug_mask(D_READER, "[cryptoworks-reader] random data check failed after decrypt");
            }
            break;
          case 0xDB: // CW
            cs_debug_mask(D_READER, "[cryptoworks-reader] nano DB (cw)");
            if(n==0x10)
            {
              memcpy(er->cw, &cta_res[i+2], 16);
              r|=1;
            }
            break;
          case 0xDF: // signature
            cs_debug_mask(D_READER, "[cryptoworks-reader] nano DF %02x (sig)", n);
            if (n==0x08)
            {
              if((cta_res[i+2]&0x50)==0x50 && !(cta_res[i+3]&0x01) && (cta_res[i+5]&0x80))
                r|=2;
            }
            else if (n==0x40) // camcrypt
            {
              if(reader->ucpk_valid)
              {
                cw_RSA(&cta_res[i+2],&cta_res[i+2], n, &reader->exp, &reader->ucpk, 0);
                cs_debug_mask(D_READER, "[cryptoworks-reader] after camcrypt ");
                r=0; secLen=n-4; n=4;
              }
              else
              {
                cs_log("[cryptoworks-reader] valid UCPK needed for camcrypt!");
                return ERROR;
              }
            }
            break;
          default:
            cs_debug_mask(D_READER, "[cryptoworks-reader] nano %02x (unhandled)",cta_res[i]);
            break;
        }
        i+=n+2;
      }
    }

/*
#ifdef LALL
    if ((cta_res[cta_lr-2]==0x9f)&&(cta_res[cta_lr-1]==0x1c))
    {
      write_cmd(insC0, NULL);
      if ((cta_lr>26)&&(cta_res[cta_lr-2]==0x90)&&(cta_res[cta_lr-1]==0))
      {
        if (rc=(((cta_res[20]&0x50)==0x50) && 
                (!(cta_res[21]&0x01)) && 
                (cta_res[23]&0x80)))
          memcpy(er->cw, cta_res+2, 16);
      }
    }
#endif
*/
  }
  //return(rc ? 1 : 0);
  return((r==3) ? 1 : 0);
}

static uint32_t cryptoworks_get_emm_provid(unsigned char *buffer, int32_t len);

static int32_t cryptoworks_get_emm_type(EMM_PACKET *ep, struct s_reader * rdr)
{
  char dumprdrserial[18];

  cs_debug_mask(D_EMM, "Entered cryptoworks_get_emm_type ep->emm[0]=%02x",ep->emm[0]);
  switch (ep->emm[0]) {
		case 0x82:
  	 		if(ep->emm[3]==0xA9 && ep->emm[4]==0xFF && ep->emm[13]==0x80 && ep->emm[14]==0x05) {
				ep->type = UNIQUE;
				memset(ep->hexserial, 0, 8);
				memcpy(ep->hexserial, ep->emm + 5, 5);
				cs_strncpy(dumprdrserial, cs_hexdump(1, rdr->hexserial, 5), sizeof(dumprdrserial));
				i2b_buf(4, cryptoworks_get_emm_provid(ep->emm+12, ep->l-12), ep->provid);
				cs_debug_mask(D_EMM, "CRYPTOWORKS EMM: UNIQUE, ep = %s rdr = %s", 
					      cs_hexdump(1, ep->hexserial, 5), dumprdrserial);
				return (!memcmp(ep->emm + 5, rdr->hexserial, 5)); // check for serial
			}
			break;
		case 0x84:
	  	 	if(ep->emm[3]==0xA9 && ep->emm[4]==0xFF && ep->emm[12]==0x80 && ep->emm[13]==0x04) {
				ep->type = SHARED;
				memset(ep->hexserial, 0, 8);
				memcpy(ep->hexserial, ep->emm + 5, 4);
				cs_strncpy(dumprdrserial, cs_hexdump(1, rdr->hexserial, 4), sizeof(dumprdrserial));
				i2b_buf(4, cryptoworks_get_emm_provid(ep->emm+12, ep->l-12), ep->provid);
				cs_debug_mask(D_EMM, "CRYPTOWORKS EMM: SHARED, ep = %s rdr = %s", 
					      cs_hexdump(1, ep->hexserial, 4), dumprdrserial);
				return (!memcmp(ep->emm + 5, rdr->hexserial, 4)); // check for SA
			}
			break;
		case 0x86:
			if(ep->emm[3]==0xA9 && ep->emm[4]==0xFF && ep->emm[5]==0x83 
			   && ep->emm[6]==0x01 && ep->emm[8]==0x85) {
				cs_debug_mask(D_EMM, "CRYPTOWORKS EMM: SHARED (Header)");
				ep->type = SHARED;
				i2b_buf(4, cryptoworks_get_emm_provid(ep->emm+8, ep->l-8), ep->provid);
				return FALSE;
			}
			break;
		case 0x88:
		case 0x89:
  	 		if(ep->emm[3]==0xA9 && ep->emm[4]==0xFF && ep->emm[8]==0x83 && ep->emm[9]==0x01) {
				cs_debug_mask(D_EMM, "CRYPTOWORKS EMM: GLOBAL");
				ep->type = GLOBAL;
				i2b_buf(4, cryptoworks_get_emm_provid(ep->emm+8, ep->l-8), ep->provid);
				return TRUE;
			}
			break;
		case 0x8F:
			ep->type = UNKNOWN;
			cs_debug_mask(D_EMM, "CRYPTOWORKS EMM: 0x8F via camd3");

			switch(ep->emm[4]) {
				case 0x44:
					i2b_buf(4, cryptoworks_get_emm_provid(ep->emm+8, ep->l-8), ep->provid);
					ep->type = GLOBAL; break;
				case 0x48:
					i2b_buf(4, cryptoworks_get_emm_provid(ep->emm+12, ep->l-12), ep->provid);
					ep->type = SHARED; break;		
				case 0x42:
					i2b_buf(4, cryptoworks_get_emm_provid(ep->emm+12, ep->l-12), ep->provid);
					ep->type = UNIQUE; break;
			}
			return TRUE;

		/* FIXME: Seems to be that all other EMM types are rejected by the card */
		default: 
			ep->type = UNKNOWN;
			cs_debug_mask(D_EMM, "CRYPTOWORKS EMM: UNKNOWN");
			return FALSE; // skip emm
	}

	cs_debug_mask(D_EMM, "CRYPTOWORKS EMM: invaild");
	return FALSE;
}

static void cryptoworks_get_emm_filter(struct s_reader * rdr, uchar *filter)
{
	int32_t idx = 2;

	filter[0]=0xFF;
	filter[1]=0;

	filter[idx++]=EMM_GLOBAL;
	filter[idx++]=0;
	filter[idx+0]    = 0x88;
	filter[idx+0+16] = 0xFE;
	filter[idx+1]    = 0xA9;
	filter[idx+1+16] = 0xFF;
	filter[idx+2]    = 0xFF;
	filter[idx+2+16] = 0xFF;
	filter[1]++;
	idx += 32;

	filter[idx++]=EMM_SHARED;
	filter[idx++]=0;
	filter[idx+0]    = 0x86;
	filter[idx+16]   = 0xFF;
	filter[idx+1]    = 0xA9;
	filter[idx+1+16] = 0xFF;
	filter[idx+2]    = 0xFF;
	filter[idx+2+16] = 0xFF;
	filter[1]++;
	idx += 32;

	filter[idx++]=EMM_SHARED;
	filter[idx++]=0;
	filter[idx+0]    = 0x84;
	filter[idx+0+16] = 0xFF;
	filter[idx+1]    = 0xA9;
	filter[idx+1+16] = 0xFF;
	filter[idx+2]    = 0xFF;
	filter[idx+2+16] = 0xFF;
	memcpy(filter+idx+3, rdr->hexserial, 4);
	memset(filter+idx+3+16, 0xFF, 4);
	filter[1]++;
	idx += 32;

	filter[idx++]=EMM_UNIQUE;
	filter[idx++]=0;
	filter[idx+0]    = 0x82;
	filter[idx+0+16] = 0xFF;
	filter[idx+1]    = 0xA9;
	filter[idx+1+16] = 0xFF;
	filter[idx+2]    = 0xFF;
	filter[idx+2+16] = 0xFF;
	memcpy(filter+idx+3, rdr->hexserial, 5);
	memset(filter+idx+3+16, 0xFF, 5);
	filter[1]++;
	idx += 32;

	return;
}

static int32_t cryptoworks_do_emm(struct s_reader * reader, EMM_PACKET *ep)
{
  def_resp;
  uchar insEMM_GA[] = {0xA4, 0x44, 0x00, 0x00, 0x00};
  uchar insEMM_SA[] = {0xA4, 0x48, 0x00, 0x00, 0x00};
  uchar insEMM_UA[] = {0xA4, 0x42, 0x00, 0x00, 0x00};
  int32_t rc=0;
  uchar *emm=ep->emm;
  
	if(emm[0]==0x8f && emm[3]==0xA4) {
		//camd3 emm	    
		write_cmd(emm+3, emm+3+CMD_LEN);
		rc=((cta_res[0]==0x90)&&(cta_res[1]==0x00));
		return(rc);	
	}


  switch(ep->type)
  {
  	 //GA    	 
  	 case GLOBAL:
				insEMM_GA[4]=ep->emm[2]-2;
				if(emm[7]==insEMM_GA[4]-3)
				{
					write_cmd(insEMM_GA, emm+5);
					rc=((cta_res[0]==0x90)&&(cta_res[1]==0x00));					
				}
  	 	break;
  	 
  	 //SA
  	 case SHARED:
				insEMM_SA[4]=ep->emm[2]-6;
				//if(emm[11]==insEMM_SA[4]-3)
				//{
					write_cmd(insEMM_SA, emm+9);
					rc=((cta_res[0]==0x90)&&(cta_res[1]==0x00));					
				//}
  	 	break;
  	 
  	 //UA	  	 
  	 case UNIQUE:
			insEMM_UA[4]=ep->emm[2]-7;
			if(emm[12]==insEMM_UA[4]-3)
			{
				//cryptoworks_send_pin(); //?? may be 
				write_cmd(insEMM_UA, emm+10);
				rc=((cta_res[0]==0x90)&&(cta_res[1]==0x00));					
			}
  	 	break;  	
  }

  if (!rc)
    cs_debug_mask(D_EMM, "cryptoworks_do_emm(): type %d - %02X %02X", ep->type, cta_res[0], cta_res[1]);

  return(rc);
}

static int32_t cryptoworks_card_info(struct s_reader * reader)
{
  def_resp;
	int32_t i;
  uchar insA21[]= {0xA4, 0xA2, 0x01, 0x00, 0x05, 0x8C, 0x00, 0x00, 0x00, 0x00};
  uchar insB2[] = {0xA4, 0xB2, 0x00, 0x00, 0x00};
  char l_name[20+8]=", name: ";

  for (i=0; i<reader->nprov; i++)
  {
    l_name[8]=0;
    select_file(reader, 0x1f, reader->prid[i][3], cta_res, &cta_lr);	// select provider
    select_file(reader, 0x0e, 0x11, cta_res, &cta_lr);		// read provider name
    if (read_record(reader, 0xD6, cta_res)>=16)
    {
      cs_strncpy(l_name+8, (const char *)cta_res+2, sizeof(l_name)-9);
      l_name[sizeof(l_name)-1]=0;
      trim(l_name+8);
    }
    l_name[0]=(l_name[8]) ? ',' : 0;
    cs_ri_log (reader, "provider: %d, id: %02X%s", i+1, reader->prid[i][3], l_name);
    select_file(reader, 0x0f, 0x20, cta_res, &cta_lr);		// select provider class
    write_cmd(insA21, insA21+5);
    if (cta_res[0]==0x9f)
    {
      insB2[4]=cta_res[1];
      for(insB2[3]=0; (cta_res[0]!=0x94)||(cta_res[1]!=0x2); insB2[3]=1)
      {
        write_cmd(insB2, NULL);		// read chid
        if (cta_res[0]!=0x94)
        {
          char ds[16], de[16];
          chid_date(cta_res+28, ds, sizeof(ds)-1);
          chid_date(cta_res+30, de, sizeof(de)-1);
          cs_ri_log (reader, "chid: %02X%02X, date: %s - %s, name: %s",
                    cta_res[6], cta_res[7], ds, de, trim((char *) cta_res+10));
        }
      }
    }

    select_file(reader, 0x0f, 0x00, cta_res, &cta_lr);		// select provider channel 
    write_cmd(insA21, insA21+5);
    if (cta_res[0]==0x9f)
    {
      insB2[4]=cta_res[1];
      for(insB2[3]=0; (cta_res[0]!=0x94)||(cta_res[1]!=0x2); insB2[3]=1)
      {
        write_cmd(insB2, NULL);		// read chid
        if (cta_res[0]!=0x94)
        {
          char ds[16], de[16];
          chid_date(cta_res+28, ds, sizeof(ds)-1);
          chid_date(cta_res+30, de, sizeof(de)-1);
          cta_res[27]=0;
          cs_ri_log (reader, "chid: %02X%02X, date: %s - %s, name: %s",
                    cta_res[6], cta_res[7], ds, de, trim((char *)cta_res+10));
        }
      }
    }
  }
  cs_log("[cryptoworks-reader] ready for requests");
  return OK;
}

static uint32_t cryptoworks_get_emm_provid(unsigned char *buffer, int32_t len)
{
    uint32_t provid=0;
    int32_t i=0;
    
    for(i=0; i<len;) {
        switch (buffer[i]) {
            case 0x83:
                provid=buffer[i+2] & 0xfc;
                return provid;
                break;
            default:
                i+=buffer[i+1]+2;
                break;
        }
        
    }
    return provid;
}

#ifdef HAVE_DVBAPI
void dvbapi_sort_nanos(unsigned char *dest, const unsigned char *src, int32_t len);

int32_t cryptoworks_reassemble_emm(uchar *buffer, uint32_t *len) {
	static uchar emm_global[512];
	static int32_t emm_global_len = 0;
	int32_t emm_len = 0;

	// Cryptoworks
	//   Cryptoworks EMM-S have to be assembled by the client from an EMM-SH with table
	//   id 0x84 and a corresponding EMM-SB (body) with table id 0x86. A pseudo EMM-S
	//   with table id 0x84 has to be build containing all nano commands from both the
	//    original EMM-SH and EMM-SB in ascending order.
	// 
	if (*len>500) return 0;
	
	switch (buffer[0]) {
		case 0x82 : // emm-u
			cs_debug_mask(D_READER, "cryptoworks unique emm (EMM-U): %s" , cs_hexdump(1, buffer, *len));
			break;

		case 0x84: // emm-sh
			cs_debug_mask(D_READER, "cryptoworks shared emm (EMM-SH): %s" , cs_hexdump(1, buffer, *len));
			if (!memcmp(emm_global, buffer, *len)) return 0;
			memcpy(emm_global, buffer, *len);
			emm_global_len=*len;
			return 0;

		case 0x86: // emm-sb
			cs_debug_mask(D_READER, "cryptoworks shared emm (EMM-SB): %s" , cs_hexdump(1, buffer, *len));
			if (!emm_global_len) return 0;

			// we keep the first 12 bytes of the 0x84 emm (EMM-SH)
			// now we need to append the payload of the 0x86 emm (EMM-SB)
			// starting after the header (&buffer[5])
			// then the rest of the payload from EMM-SH
			// so we should have :
			// EMM-SH[0:12] + EMM-SB[5:len_EMM-SB] + EMM-SH[12:EMM-SH_len]
			// then sort the nano in ascending order
			// update the emm len (emmBuf[1:2])
			//

			emm_len=*len-5 + emm_global_len-12;
			unsigned char *tmp, *assembled;
			if(!cs_malloc(&tmp,emm_len, -1)) return 0;
			if(!cs_malloc(&assembled,emm_len+12, -1)){
				free(tmp);
				return 0;
			}
			unsigned char *assembled_EMM=malloc(emm_len+12);
			memcpy(tmp,&buffer[5], *len-5);
			memcpy(tmp+*len-5,&emm_global[12],emm_global_len-12);
			memcpy(assembled_EMM,emm_global,12);
			dvbapi_sort_nanos(assembled_EMM+12,tmp,emm_len);

			assembled_EMM[1]=((emm_len+9)>>8) | 0x70;
			assembled_EMM[2]=(emm_len+9) & 0xFF;
			//copy back the assembled emm in the working buffer
			memcpy(buffer, assembled_EMM, emm_len+12);
			*len=emm_len+12;

			free(tmp);
			free(assembled_EMM);

			emm_global_len=0;

			cs_debug_mask(D_READER, "cryptoworks shared emm (assembled): %s" , cs_hexdump(1, buffer, emm_len+12));
			if(assembled_EMM[11]!=emm_len) { // sanity check
				// error in emm assembly
				cs_debug_mask(D_READER, "Error assembling Cryptoworks EMM-S");
				return 0;
			}
			break;
				
		case 0x88: // emm-g
		case 0x89: // emm-g
			cs_debug_mask(D_READER, "cryptoworks global emm (EMM-G): %s" , cs_hexdump(1, buffer, *len));
			break;
	}
	return 1;
}
#endif

void reader_cryptoworks(struct s_cardsystem *ph) 
{
	ph->do_emm=cryptoworks_do_emm;
	ph->do_ecm=cryptoworks_do_ecm;
	ph->card_info=cryptoworks_card_info;
	ph->card_init=cryptoworks_card_init;
	ph->get_emm_type=cryptoworks_get_emm_type;
	ph->get_emm_filter=cryptoworks_get_emm_filter;
	ph->caids[0]=0x0D;
	ph->desc="cryptoworks";
}
