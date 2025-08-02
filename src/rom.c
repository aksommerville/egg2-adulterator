#include "adl.h"

/* Cleanup.
 */
 
void rom_cleanup(struct rom *rom) {
  if (rom->resv) free(rom->resv);
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

/* Decode.
 */
 
int rom_decode(struct rom *rom,const void *src,int srcc,const char *refname) {
  if (rom->resc) return -1;
  fprintf(stderr,"TODO: %s from %s, %d bytes\n",__func__,refname,srcc);
  return -2;
}

/* Encode.
 */
 
int rom_encode(struct buffer *dst,struct rom *rom,const char *refname) {
  fprintf(stderr,"TODO: %s for %s, %d resources\n",__func__,refname,rom->resc);
  return -2;
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
