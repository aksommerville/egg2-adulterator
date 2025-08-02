#include "adl.h"

/* Cleanup.
 */
 
void rom_cleanup(struct rom *rom) {
  if (rom->resv) free(rom->resv);
  if (rom->extra) free(rom->extra);
}

/* Grow resv.
 */
 
static int rom_resv_require(struct rom *rom) {
  if (rom->resc<rom->resa) return 0;
  int na=rom->resa+128;
  if (na>INT_MAX/sizeof(struct res)) return -1;
  void *nv=realloc(rom->resv,sizeof(struct res)*na);
  if (!nv) return -1;
  rom->resv=nv;
  rom->resa=na;
  return 0;
}

/* Decode from a binary ROM.
 * Caller may set (prefix,suffix) first, we won't touch them.
 */
 
static int rom_decode_binary(struct rom *rom,const uint8_t *src,int srcc,const char *refname) {
  if (rom->resc) return -1;
  #define FAIL(fmt,...) { \
    if (!refname) return -1; \
    fprintf(stderr,"%s: "fmt"\n",refname,##__VA_ARGS__); \
    return -2; \
  }
  if ((srcc<4)||memcmp(src,"\0ERM",4)) FAIL("Signature mismatch.")
  int srcp=4,tid=1,rid=1;
  for (;;) {
    if (srcp>=srcc) FAIL("Missing terminator.")
    uint8_t lead=src[srcp++];
    if (!lead) break;
    switch (lead&0xc0) {
      case 0x00: { // TID
          tid+=lead;
          rid=1;
          if (tid>0xff) FAIL("TID overflow.")
        } break;
      case 0x40: { // RID
          if (srcp>srcc-1) FAIL("Unexpected EOF.")
          int d=(lead&0x3f)<<8;
          d|=src[srcp++];
          rid+=d+1;
          if (rid>0xffff) FAIL("RID overflow.")
        } break;
      case 0x80: { // RES
          if (srcp>srcc-2) FAIL("Unexpected EOF.")
          int l=(lead&0x3f)<<16;
          l|=src[srcp++]<<8;
          l|=src[srcp++];
          l++;
          if (srcp>srcc-l) FAIL("Unexpected EOF.")
          if (rid>0xffff) FAIL("RID overflow.")
          if (rom_resv_require(rom)<0) return -1;
          struct res *res=rom->resv+rom->resc++;
          res->tid=tid;
          res->rid=rid;
          res->v=src+srcp;
          res->c=l;
          srcp+=l;
          rid++;
        } break;
      default: FAIL("Invalid leading byte 0x%02x.",lead)
    }
  }
  if ((srcp<srcc)&&refname) {
    fprintf(stderr,"%s:WARNING: Ignoring %d bytes of ROM after terminator.\n",refname,srcc-srcp);
  }
  #undef FAIL
  return 0;
}

/* Measure binary.
 * A valid binary is at least 5 bytes.
 */
 
static int rom_measure_binary(const uint8_t *src,int srcc) {
  if (srcc<5) return 0;
  if (memcmp(src,"\0ERM",4)) return 0;
  int srcp=4;
  for (;;) {
    if (srcp>=srcc) return 0; // Missing required terminator.
    uint8_t lead=src[srcp++];
    if (!lead) return srcp; // Terminated.
    switch (lead&0xc0) { // OK to skip payloads even if we exceed EOF, we don't have to read them.
      case 0x00: break; // TID
      case 0x40: srcp+=1; break; // RID
      case 0x80: srcp+=2; break; // RES
      default: return 0; // Illegal.
    }
  }
  return 0;
}

/* Locate the embedded ROM and decode it.
 */
 
static int rom_decode_exe(struct rom *rom,const uint8_t *src,int srcc,const char *refname) {
  int srcp=0,bestp=0,bestlen=0;
  while (srcp<srcc) {
    int len=rom_measure_binary(src+srcp,srcc-srcp);
    if (len<1) {
      srcp++;
      continue;
    }
    // It's not unusual to find dummy empty ROMs "\0ERM\0" in an executable.
    // Use those only if we don't find a longer one.
    if (len>bestlen) {
      bestp=srcp;
      bestlen=len;
      if (len>5) break;
    }
    srcp+=len;
  }
  if (bestlen<5) {
    fprintf(stderr,"%s: Unable to locate ROM in %d-byte file.\n",refname,srcc);
    return -2;
  }
  rom->prefix=src;
  rom->prefixc=bestp;
  rom->suffix=src+bestp+bestlen;
  rom->suffixc=srcc-bestlen-bestp;
  return rom_decode_binary(rom,src+bestp,bestlen,refname);
}

/* Locate ROM in HTML, decode base64, then decode the ROM.
 */
 
static int rom_decode_html(struct rom *rom,const char *src,int srcc,const char *refname) {
  int srcp=0,startp=-1;
  for (;srcp<srcc;srcp++) {
    if (srcp>srcc-9) break;
    if (!memcmp(src+srcp,"<egg-rom>",9)) {
      startp=srcp+9;
      break;
    }
  }
  if (startp<0) {
    fprintf(stderr,"%s: Failed to locate <egg-rom> tag.\n",refname);
    return -2;
  }
  int endp=-1;
  for (srcp=startp;srcp<srcc;srcp++) {
    if (srcp>srcc-10) break;
    if (!memcmp(src+srcp,"</egg-rom>",10)) {
      endp=srcp;
      break;
    }
  }
  if (endp<0) {
    fprintf(stderr,"%s: Unclosed <egg-rom> tag.\n",refname);
    return -2;
  }
  rom->prefix=src;
  rom->prefixc=startp;
  rom->suffix=src+endp;
  rom->suffixc=srcc-endp;
  rom->base64=1;
  struct buffer bin={0};
  int err=buffer_decode_base64(&bin,src+startp,endp-startp,refname);
  if (err<0) {
    buffer_cleanup(&bin);
    if (err!=-2) fprintf(stderr,"%s: Failed to decode base-64.\n",refname);
    return -2;
  }
  rom->extra=bin.v; // Handoff to (extra); rom will free it later.
  return rom_decode_binary(rom,bin.v,bin.c,refname);
}

/* Decode.
 */
 
int rom_decode(struct rom *rom,const void *src,int srcc,const char *refname) {
  if (rom->resc) return -1;
  if ((srcc>=9)&&!memcmp(src,"<!DOCTYPE",9)) return rom_decode_html(rom,src,srcc,refname);
  if ((srcc>=4)&&!memcmp(src,"\0ERM",4)) return rom_decode_binary(rom,src,srcc,refname);
  return rom_decode_exe(rom,src,srcc,refname);
}

/* Encode, no framing or extra encoding.
 */
 
static int rom_encode_binary(struct buffer *dst,const struct rom *rom,const char *refname) {
  buffer_raw(dst,"\0ERM",4);
  int tid=1,rid=1,i=rom->resc;
  const struct res *res=rom->resv;
  #define FAIL(fmt,...) { \
    if (!refname) return -1; \
    fprintf(stderr,"%s: "fmt"\n",refname,##__VA_ARGS__); \
    return -2; \
  }
  for (;i-->0;res++) {
  
    // Skip empties. They're not just redundant; there is actually no way to express them.
    if (res->c<1) continue;
  
    // Advance tid if required.
    int dtid=res->tid-tid;
    if (dtid<0) FAIL("Resources not sorted (tid %d<%d).",res->tid,tid)
    while (dtid>=0x3f) {
      buffer_u8(dst,0x3f);
      dtid-=0x3f;
      rid=1;
    }
    if (dtid>0) {
      buffer_u8(dst,dtid);
      rid=1;
    }
    tid=res->tid;
  
    // Advance rid if required.
    int drid=res->rid-rid;
    if (drid<0) FAIL("Resources not sorted (tid %d, rid %d<%d).",tid,res->rid,rid)
    while (drid>=0x4000) {
      buffer_raw(dst,"\x7f\xff",2);
      drid-=0x4000;
    }
    if (drid>0) {
      drid--;
      uint8_t tmp[2]={0x40|(drid>>8),drid};
      buffer_raw(dst,tmp,2);
    }
    rid=res->rid;
    
    // Emit resource.
    if (res->c>0x400000) FAIL("Resource %d:%d too long (%d>%d).",tid,rid,res->c,0x400000)
    int l=res->c-1;
    uint8_t tmp[3]={0x80|(l>>16),l>>8,l};
    buffer_raw(dst,tmp,3);
    buffer_raw(dst,res->v,res->c);
    rid++;
  }
  buffer_u8(dst,0x00); // Terminator is required.
  #undef FAIL
  return buffer_require(dst,0);
}

/* Encode.
 */
 
int rom_encode(struct buffer *dst,struct rom *rom,const char *refname) {
  buffer_raw(dst,rom->prefix,rom->prefixc);
  if (rom->base64) {
    struct buffer bin={0};
    int err=rom_encode_binary(&bin,rom,refname);
    if (err<0) {
      buffer_cleanup(&bin);
      return err;
    }
    buffer_encode_base64(dst,bin.v,bin.c);
  } else {
    int err=rom_encode_binary(dst,rom,refname);
    if (err<0) return err;
  }
  buffer_raw(dst,rom->suffix,rom->suffixc);
  return buffer_require(dst,0);
}

/* tid and rid from path.
 */
 
static int rom_parse_path(int *tid,int *rid,const char *path) {
  const char *base=path,*prev=0;
  int basec=0,prevc=0,pathc=0;
  for (;path[pathc];pathc++) {
    if ((path[pathc]=='/')||(path[pathc]=='\\')) {
      prev=base;
      prevc=basec;
      base=path+pathc+1;
      basec=0;
    } else {
      basec++;
    }
  }
  
  // Custom tid are not supported; only the standard named types.
  if ((prevc==8)&&!memcmp(prev,"metadata",8)) *tid=1;
  else if ((prevc==4)&&!memcmp(prev,"code",4)) *tid=2;
  else if ((prevc==7)&&!memcmp(prev,"strings",7)) *tid=3;
  else if ((prevc==5)&&!memcmp(prev,"image",5)) *tid=4;
  else if ((prevc==4)&&!memcmp(prev,"song",4)) *tid=5;
  else if ((prevc==5)&&!memcmp(prev,"sound",5)) *tid=6;
  else if ((prevc==9)&&!memcmp(prev,"tilesheet",9)) *tid=7;
  else if ((prevc==10)&&!memcmp(prev,"decalsheet",10)) *tid=8;
  else if ((prevc==3)&&!memcmp(prev,"map",3)) *tid=9;
  else if ((prevc==6)&&!memcmp(prev,"sprite",6)) *tid=10;
  else return -1;
  
  *rid=0;
  int i=0;
  for (;i<basec;i++) {
    if ((base[i]=='-')||(base[i]=='.')) break;
    if ((base[i]<'0')||(base[i]>'9')) return -1;
    (*rid)*=10;
    (*rid)+=base[i]-'0';
    if (*rid>0xffff) return -1;
  }
  if (!*rid) return -1;
  
  return 0;
}

/* Replace one resource.
 */
 
int rom_replace(struct rom *rom,const void *src,int srcc,const char *srcpath) {
  if (!srcpath) return -1;
  if ((srcc<0)||(srcc&&!src)) return -1;
  
  // Determine IDs.
  int tid=0,rid=0,err;
  if ((err=rom_parse_path(&tid,&rid,srcpath))<0) {
    if (err!=-2) fprintf(stderr,"%s: Failed to determine type and ID from path.\n",srcpath);
    return -2;
  }
  
  // Replace existing, or determine insertion point.
  struct res *res=rom->resv;
  int p=0;
  for (;p<rom->resc;res++) {
    if (res->tid>tid) break;
    if (res->tid<tid) continue;
    if (res->rid>rid) break;
    if (res->rid<rid) continue;
    res->v=src;
    res->c=srcc;
    return 0;
  }
  
  // Insert.
  if (rom_resv_require(rom)<0) return -1;
  res=rom->resv+p;
  memmove(res+1,res,sizeof(struct res)*(rom->resc-p));
  rom->resc++;
  res->tid=tid;
  res->rid=rid;
  res->v=src;
  res->c=srcc;
  
  return 0;
}
