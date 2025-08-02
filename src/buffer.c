#include "adl.h"

/* Storage concerns.
 */
 
void buffer_cleanup(struct buffer *buffer) {
  if (buffer->v) free(buffer->v);
}

int buffer_require(struct buffer *buffer,int addc) {
  if (buffer->err<0) return buffer->err;
  if (addc<1) return 0;
  if (buffer->c>INT_MAX-addc) return buffer->err=-1;
  int na=buffer->c+addc;
  if (na<=buffer->a) return 0;
  if (na<INT_MAX-1024) na=(na+1024)&~1023;
  void *nv=realloc(buffer->v,na);
  if (!nv) return buffer->err=-1;
  buffer->v=nv;
  buffer->a=na;
  return 0;
}

/* Simple appends.
 */

int buffer_raw(struct buffer *buffer,const void *src,int srcc) {
  if (!src) srcc=0; else if (srcc<0) { srcc=0; while (((char*)src)[srcc]) srcc++; }
  if (buffer_require(buffer,srcc)<0) return -1;
  memcpy(buffer->v+buffer->c,src,srcc);
  buffer->c+=srcc;
  return 0;
}

int buffer_u8(struct buffer *buffer,uint8_t v) {
  if (buffer_require(buffer,1)<0) return -1;
  buffer->v[buffer->c++]=v;
  return 0;
}

/* Base64.
 */

int buffer_decode_base64(struct buffer *buffer,const char *src,int srcc,const char *refname) {
  uint8_t tmp[4]; // 0..63
  int srcp=0,tmpc=0;
  while (srcp<srcc) {
    if ((unsigned char)src[srcp]<=0x20) {
      srcp++;
      continue;
    }
    if (src[srcp]=='=') {
      srcp++;
      continue;
    }
    int digit=src[srcp++];
         if ((digit>='A')&&(digit<='Z')) digit=digit-'A';
    else if ((digit>='a')&&(digit<='z')) digit=digit-'a'+26;
    else if ((digit>='0')&&(digit<='9')) digit=digit-'0'+52;
    else if (digit=='+') digit=62;
    else if (digit=='/') digit=63;
    else {
      if (!refname) return -1;
      fprintf(stderr,"%s: Unexpected character '%c' (0x%02x) in base64.\n",refname,digit,digit);
      return -2;
    }
    tmp[tmpc++]=digit;
    if (tmpc>=4) {
      uint8_t out[3]={
        (tmp[0]<<2)|(tmp[1]>>4),
        (tmp[1]<<4)|(tmp[2]>>2),
        (tmp[2]<<6)|tmp[3],
      };
      if (buffer_raw(buffer,out,3)<0) return -1;
      tmpc=0;
    }
  }
  switch (tmpc) {
    case 1: { // not supposed to happen, is it an error?
        uint8_t out[3]={
          (tmp[0]<<2),
        };
        if (buffer_raw(buffer,out,1)<0) return -1;
      } break;
    case 2: {
        uint8_t out[2]={
          (tmp[0]<<2)|(tmp[1]>>4),
          (tmp[1]<<4),
        };
        if (buffer_raw(buffer,out,1)<0) return -1;
      } break;
    case 3: {
        uint8_t out[3]={
          (tmp[0]<<2)|(tmp[1]>>4),
          (tmp[1]<<4)|(tmp[2]>>2),
          (tmp[2]<<6),
        };
        if (buffer_raw(buffer,out,2)<0) return -1;
      } break;
  }
  return 0;
}

int buffer_encode_base64(struct buffer *buffer,const void *src,int srcc) {
  const uint8_t *SRC=src;
  const char *alphabet="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  int srcp=0,stopp=srcc-3;
  int linep=buffer->c;
  while (srcp<=stopp) {
    uint8_t a=SRC[srcp++];
    uint8_t b=SRC[srcp++];
    uint8_t c=SRC[srcp++];
    char tmp[4]={
      alphabet[a>>2],
      alphabet[((a<<4)|(b>>4))&0x3f],
      alphabet[((b<<2)|(c>>6))&0x3f],
      alphabet[c&0x3f],
    };
    if (buffer_raw(buffer,tmp,4)<0) return -1;
    if (buffer->c-linep>=100) {
      buffer_u8(buffer,0x0a);
      linep=buffer->c;
    }
  }
  switch (srcc-srcp) {
    case 1: {
        uint8_t a=SRC[srcp++];
        char tmp[4]={
          alphabet[a>>2],
          alphabet[(a<<4)&0x3f],
          '=','=',
        };
        if (buffer_raw(buffer,tmp,4)<0) return -1;
      } break;
    case 2: {
        uint8_t a=SRC[srcp++];
        uint8_t b=SRC[srcp++];
        char tmp[4]={
          alphabet[a>>2],
          alphabet[((a<<4)|(b>>4))&0x3f],
          alphabet[(b<<2)&0x3f],
          '=',
        };
        if (buffer_raw(buffer,tmp,4)<0) return -1;
      } break;
  }
  return 0;
}
