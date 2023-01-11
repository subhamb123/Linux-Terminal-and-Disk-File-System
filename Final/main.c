/****************************************************************************
*                   KCW: mount root file system                             *
*****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>

#include <ext2fs/ext2_fs.h>

#include <string.h>
#include <libgen.h>
#include <sys/stat.h>
#include <time.h>

#include "type.h"

SUPER *sp; 
GD    *gp; 
INODE *ip; 
DIR   *dp; 

extern MINODE *iget();

MINODE minode[NMINODE];
MINODE *root;
PROC   proc[NPROC], *running;

char gpath[128]; 
char *name[64];  
int   n;         

int  fd, dev;
int  nblocks, ninodes, bmap, imap, iblk;
char line[128], cmd[32], pathname[128], old[64], new[64];

OFT oft[64];

#include "cd_ls_pwd.c"
#include "mkdir_creat.c"
#include "rmdir.c"
#include "link_unlink.c"
#include "symlink.c"
#include "open_close_lseek.c"
#include "read_cat.c"
#include "write_cp.c"

int init()
{
  int i, j;
  MINODE *mip;
  PROC   *p;

  printf("init()\n");

  for (i=0; i<NMINODE; i++){
    mip = &minode[i];
    mip->dev = mip->ino = 0;
    mip->refCount = 0;
    mip->mounted = 0;
    mip->mptr = 0;
  }
  for (i=0; i<NPROC; i++){
    p = &proc[i];
    p->pid = i+1;           
    p->uid = p->gid = 0;    
    p->cwd = 0;             

    //No opened file yet
    for (j = 0; j < NFD; j++) {
        p->fd[j] = 0;
        //p->fd[j]->refCount = 0;
    }
  }
}

int mount_root()
{  
  printf("mount_root()\n");
  root = iget(dev, 2);
}

char *disk = "disk2";     // change this to YOUR virtual

int main(int argc, char *argv[ ])
{
  int ino;
  char buf[BLKSIZE];

  printf("checking EXT2 FS ....");
  if ((fd = open(disk, O_RDWR)) < 0){
    printf("open %s failed\n", disk);
    exit(1);
  }

  dev = fd;    // global dev same as this fd   

  /********** read super block  ****************/
  get_block(dev, 1, buf);
  sp = (SUPER *)buf;

  /* verify it's an ext2 file system ***********/
  if (sp->s_magic != 0xEF53){
      printf("magic = %x is not an ext2 filesystem\n", sp->s_magic);
      exit(1);
  }     
  printf("EXT2 FS OK\n");
  ninodes = sp->s_inodes_count;
  nblocks = sp->s_blocks_count;

  get_block(dev, 2, buf); 
  gp = (GD *)buf;

  bmap = gp->bg_block_bitmap;
  imap = gp->bg_inode_bitmap;
  iblk = gp->bg_inode_table;
  printf("bmp=%d imap=%d inode_start = %d\n", bmap, imap, iblk);

  init();  
  mount_root();
  printf("root refCount = %d\n", root->refCount);

  printf("creating P0 as running process\n");
  running = &proc[0];
  running->cwd = iget(dev, 2);
  printf("root refCount = %d\n", root->refCount);
  
  while(1){
    printf("input command : [ls|cd|pwd|mkdir|creat|rmdir|link|unlink|symlink|open|close|\nread|cat|write|cp|quit] ");
    fgets(line, 128, stdin);
    line[strlen(line)-1] = 0;

    if (line[0]==0)
       continue;
    pathname[0] = 0;

    int temp = 1;

    //sscanf(line, "%s %s", cmd, pathname);
    for(int i = 0; line[i] != '\0'; i++){
        if(line[i] == ' '){
            strcpy(cmd, strtok(line, " "));
            strcpy(pathname, strtok(NULL, "\0"));
            temp = 0;
            break;
        }
    }

    if(temp)
        sscanf(line, "%s %s", cmd, pathname);

    printf("cmd=%s pathname=%s\n", cmd, pathname);
  
    if (strcmp(cmd, "ls")==0)
       ls();
    else if (strcmp(cmd, "cd")==0)
       cd();
    else if (strcmp(cmd, "pwd")==0)
       pwd(running->cwd);
    else if (strcmp(cmd, "mkdir")==0)
        mymkdir(pathname);
    else if (strcmp(cmd, "creat")==0)
        mycreat(pathname);
    else if (strcmp(cmd, "rmdir")==0)
        rmdir(pathname);
    else if (strcmp(cmd, "link")==0) {
        sscanf(pathname, "%s %s", old, new);
        link(old, new);
    }
    else if (strcmp(cmd, "unlink")==0)
        unlink(pathname);
    else if (strcmp(cmd, "symlink")==0) {
        sscanf(pathname, "%s %s", old, new);
        symlink(old, new);
    }
    else if (strcmp(cmd, "read") == 0){
        read_file();
    }
    else if (strcmp(cmd, "cat") == 0){
       my_cat(pathname);
    }
    else if (strcmp(cmd, "open")==0){
        printf("Enter <filename> <mode (0,1,2,3 for R,W,RW,A)>");
        fgets(line, 128, stdin);
        line[strlen(line)-1] = 0;
        if (line[0]==0)
            return -1;
        pathname[0] = 0;
        strcpy(cmd, strtok(line, " "));
        strcpy(pathname, strtok(NULL, "\0"));
        int mode = atoi(pathname);
        fd = open_file(cmd, mode);
        printf("fd = %d\n", fd);
    }
        
    else if (strcmp(cmd, "close")==0)
        close_file(fd);

    else if (strcmp(cmd, "write")==0){
        sscanf(pathname, "%s %s", old, new);
        write_file(atoi(old), new);
    }
    else if (strcmp(cmd, "cp")==0) {
        sscanf(pathname, "%s %s", old, new);
        mycp(old, new);
    }
    else if (strcmp(cmd, "quit")==0)
       quit();
  }
}

int quit()
{
  int i;
  MINODE *mip;
  for (i=0; i<NMINODE; i++){
    mip = &minode[i];
    if (mip->refCount > 0)
      iput(mip);
  }
  printf("see you later, alligator\n");
  exit(0);
}
