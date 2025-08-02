#include "adl.h"

struct g g={0};

/* --help
 */
 
static void print_help() {
  fprintf(stderr,"Usage: %s ROM RESFILE\n",g.exename);
  fprintf(stderr,"Rewrite ROM, inserting or replacing resource RESFILE.\n");
  fprintf(stderr,"RESFILE's path as provided must name its type and ID. eg 'src/data/image/1-myimage.png'.\n");
}

/* Main.
 */
 
int main(int argc,char **argv) {
  int err;

  g.exename="egg2-adulterator";
  if ((argc>=1)&&argv&&argv[0]&&argv[0][0]) g.exename=argv[0];
  int argi=1;
  while (argi<argc) {
    const char *arg=argv[argi++];
    if (!arg||!arg[0]) continue;
    
    if (!strcmp(arg,"--help")) {
      print_help();
      return 0;
    }
    
    if (arg[0]=='-') {
      fprintf(stderr,"%s: Unexpected option '%s'\n",g.exename,arg);
      return 1;
    }
    
    if (!g.dstpath) {
      g.dstpath=arg;
      continue;
    }
    
    if (!g.srcpath) {
      g.srcpath=arg;
      continue;
    }
    
    fprintf(stderr,"%s: Unexpected argument '%s'\n",g.exename,arg);
    return 1;
  }
  if (!g.dstpath||!g.srcpath) {
    print_help();
    return 1;
  }
  
  void *src=0;
  int srcc=file_read(&src,g.dstpath);
  if (srcc<0) {
    fprintf(stderr,"%s: Failed to read file.\n",g.dstpath);
    return 1;
  }
  
  struct rom rom={0};
  if ((err=rom_decode(&rom,src,srcc,g.dstpath))<0) {
    if (err!=-2) fprintf(stderr,"%s: Unspecified error decoding %d-byte ROM.\n",g.dstpath,srcc);
    return 1;
  }
  
  struct buffer incoming={0};
  if (file_read(&incoming,g.srcpath)<0) {
    fprintf(stderr,"%s: Failed to read file.\n",g.srcpath);
    return 1;
  }
  
  if ((err=rom_replace(&rom,incoming.v,incoming.c,g.srcpath))<0) {
    if (err!=-2) fprintf(stderr,"%s: Unspecified error inserting %d-byte resource.\n",g.srcpath,incoming.c);
    return 1;
  }
  
  struct buffer dst={0};
  if ((err=rom_encode(&dst,&rom,g.dstpath))<0) {
    if (err!=-2) fprintf(stderr,"%s: Unspecified error re-encoding ROM.\n",g.dstpath);
    return 1;
  }
  
  if (file_write(g.dstpath,dst.v,dst.c)<0) {
    fprintf(stderr,"%s: Failed to rewrite file, %d bytes.\n",g.dstpath,dst.c);
    return 1;
  }
  
  return 0;
}
