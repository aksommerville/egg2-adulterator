#include "adl.h"

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
