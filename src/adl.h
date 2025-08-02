#ifndef ADL_H
#define ADL_H

#include <string.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>

#ifndef O_BINARY
  #define O_BINARY 0
#endif

extern struct g {
  const char *exename;
  const char *dstpath;
  const char *srcpath;
} g;

struct buffer {
  uint8_t *v;
  int c,a,err;
};

void buffer_cleanup(struct buffer *buffer);
int buffer_require(struct buffer *buffer,int addc);
int buffer_raw(struct buffer *buffer,const void *src,int srcc);
int buffer_u8(struct buffer *buffer,uint8_t v);
int buffer_decode_base64(struct buffer *buffer,const char *src,int srcc,const char *refname);
int buffer_encode_base64(struct buffer *buffer,const void *src,int srcc);

int file_read(void *dstpp,const char *path);
int file_write(const char *path,const void *src,int srcc);

struct rom {
  struct res {
    int tid,rid;
    const void *v;
    int c;
  } *resv;
  int resc,resa;
  const void *prefix,*suffix;
  int prefixc,suffixc;
  int base64;
  void *extra;
};

/* Any input you provide to rom_decode or rom_replace must be held constant thoughout rom's life.
 */
void rom_cleanup(struct rom *rom);
int rom_decode(struct rom *rom,const void *src,int srcc,const char *refname);
int rom_encode(struct buffer *dst,struct rom *rom,const char *refname);
int rom_replace(struct rom *rom,const void *src,int srcc,const char *srcpath);

#endif
