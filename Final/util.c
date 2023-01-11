/*********** util.c file ****************/
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <ext2fs/ext2_fs.h>
#include <string.h>
#include <libgen.h>
#include <sys/stat.h>
#include <time.h>

#include "type.h"

/**** globals defined in main.c file ****/
extern MINODE minode[NMINODE];
extern MINODE *root;
extern PROC   proc[NPROC], *running;

extern char gpath[128];
extern char *name[64];
extern int n;

extern int fd, dev;
extern int nblocks, ninodes, bmap, imap, iblk;

extern char line[128], cmd[32], pathname[128];

int get_block(int dev, int blk, char *buf)
{
   lseek(dev, (long)blk*BLKSIZE, 0);
   read(dev, buf, BLKSIZE);
}   

int put_block(int dev, int blk, char *buf)
{
   lseek(dev, (long)blk*BLKSIZE, 0);
   write(dev, buf, BLKSIZE);
}   

int tokenize(char *pathname)
{
  int i;
  char *s;
  printf("tokenize %s\n", pathname);

  strcpy(gpath, pathname);   
  n = 0;

  s = strtok(gpath, "/");
  while(s){
    name[n] = s;
    n++;
    s = strtok(0, "/");
  }
  name[n] = 0;
  
  for (i= 0; i<n; i++)
    printf("%s  ", name[i]);
  printf("\n");
}

MINODE *iget(int dev, int ino)
{
  int i;
  MINODE *mip;
  char buf[BLKSIZE];
  int blk, offset;
  INODE *ip;

  for (i=0; i<NMINODE; i++){
    mip = &minode[i];
    if (mip->refCount && mip->dev == dev && mip->ino == ino){
       mip->refCount++;
       //printf("found [%d %d] as minode[%d] in core\n", dev, ino, i);
       return mip;
    }
  }
    
  for (i=0; i<NMINODE; i++){
    mip = &minode[i];
    if (mip->refCount == 0){
       //printf("allocating NEW minode[%d] for [%d %d]\n", i, dev, ino);
       mip->refCount = 1;
       mip->dev = dev;
       mip->ino = ino;

       blk    = (ino-1)/8 + iblk;
       offset = (ino-1) % 8;

       //printf("iget: ino=%d blk=%d offset=%d\n", ino, blk, offset);

       get_block(dev, blk, buf);    
       ip = (INODE *)buf + offset;  
       
       mip->INODE = *ip;
       return mip;
    }
  }   
  printf("PANIC: no more free minodes\n");
  return 0;
}

void iput(MINODE *mip)  
{
 int i, block, offset, iblock;
 char buf[BLKSIZE];
 INODE *ip;
 

 if (mip==0) 
     return;

 mip->refCount--;
 
 if (mip->refCount > 0) return;
 if (!mip->dirty)       return;
 
 /* write INODE back to disk */
 /**************** NOTE ******************************
  For mountroot, we never MODIFY any loaded INODE
                 so no need to write it back
  FOR LATER WROK: MUST write INODE back to disk if refCount==0 && DIRTY

  Write YOUR code here to write INODE back to disk
 *****************************************************/

   block = ((mip->ino - 1) / 8) + iblk;
   offset = (mip->ino - 1) % 8;

   get_block(mip->dev, block, buf);
  
   ip = (INODE *)buf + offset; 
   *ip = mip->INODE;

   put_block(mip->dev, block, buf);
} 

int search(MINODE *mip, char *name)
{
   int i; 
   char *cp, c, sbuf[BLKSIZE], temp[256];
   DIR *dp;
   INODE *ip;

   printf("search for %s in MINODE = [%d, %d]\n", name,mip->dev,mip->ino);
   ip = &(mip->INODE);


   get_block(dev, ip->i_block[0], sbuf);
   dp = (DIR *)sbuf;
   cp = sbuf;
   printf("  ino   rlen  nlen  name\n");

   while (cp < sbuf + BLKSIZE){
     strncpy(temp, dp->name, dp->name_len); 
     temp[dp->name_len] = 0;               
     printf("%4d  %4d  %4d    %s\n", dp->inode, dp->rec_len, dp->name_len, temp); 

     if (strcmp(temp, name)==0){            
        printf("found %s : ino = %d\n", temp, dp->inode);
        return dp->inode;
     }

     cp += dp->rec_len;
     dp = (DIR *)cp;
   }
   return 0;
}

int getino(char *pathname) 
{
  int i, ino, blk, offset;
  char buf[BLKSIZE];
  INODE *ip;
  MINODE *mip;

  printf("getino: pathname=%s\n", pathname);
  if (strcmp(pathname, "/")==0)
      return 2;
  
  if (pathname[0]=='/')
     mip = root;
  else
     mip = running->cwd;

  mip->refCount++;         
  
  tokenize(pathname);

  for (i=0; i<n; i++){
      printf("===========================================\n");
      printf("getino: i=%d name[%d]=%s\n", i, i, name[i]);
 
      ino = search(mip, name[i]);

      if (ino==0){
         iput(mip);
         printf("name %s does not exist\n", name[i]);
         return 0;
      }

      iput(mip);
      mip = iget(dev, ino);
   }

   iput(mip);
   return ino;
}

int findmyname(MINODE *parent, u32 myino, char myname[ ]) 
{
  char *cp, c, sbuf[BLKSIZE], temp[256];
    DIR *dp;
    MINODE *ip = parent;

    for (int i = 0; i < 12; ++i) {

        if (ip->INODE.i_block[i] != 0) {

            get_block(ip->dev, ip->INODE.i_block[i], sbuf);

            dp = (DIR *) (sbuf); 
            cp = sbuf;

            while (cp < sbuf + BLKSIZE) {
                if (dp->inode == myino) {
                    strncpy(myname, dp->name, dp->name_len);
                    myname[dp->name_len] = 0; 

                    return 0;
                }

                cp += dp->rec_len;
                dp = (DIR *) (cp);
            }
        }
    }

    return -1;
}



int findino(MINODE *mip, u32 *myino)
{
  char buffer[BLKSIZE], *cp;
  DIR *dp;
  
  get_block(mip->dev, mip->INODE.i_block[0], buffer);
  
   dp = (DIR *) buffer;
   cp = buffer;

   *myino = dp->inode;

   cp += dp->rec_len;

   dp = (DIR *) cp;

   return dp->inode;
}

int enter_name(MINODE *pip, int myino, char *myname)
{
    char buf[BLKSIZE], *cp;
    int bno;
    INODE *ip;
    DIR *dp;

    int need_len = 4 * ((8 + strlen(myname) + 3) / 4); 

    ip = &pip->INODE; 

    for (int i = 0; i < 12; i++)
    {

        if (ip->i_block[i] == 0)
        {
            break;
        }

        bno = ip->i_block[i];
        get_block(pip->dev, ip->i_block[i], buf); 
        dp = (DIR *)buf;
        cp = buf;

        while (cp + dp->rec_len < buf + BLKSIZE) 
        {
            printf("%s\n", dp->name);
            cp += dp->rec_len;
            dp = (DIR *)cp;
        }

        int ideal_len = 4 * ((8 + dp->name_len + 3) / 4); 
        int remainder = dp->rec_len - ideal_len;          

        if (remainder >= need_len)
        {                            
            dp->rec_len = ideal_len; 
            cp += dp->rec_len;      
            dp = (DIR *)cp;          

            dp->inode = myino;             
            strcpy(dp->name, myname);     
            dp->name_len = strlen(myname); 
            dp->rec_len = remainder;       

            put_block(dev, bno, buf);
            return 0;
        }
        else
        {                        
            ip->i_size = BLKSIZE; 
            bno = balloc(dev);    
            ip->i_block[i] = bno; 
            pip->dirty = 1;       

            get_block(dev, bno, buf); 
            dp = (DIR *)buf;
            cp = buf;

            dp->name_len = strlen(myname);
            strcpy(dp->name, myname);      
            dp->inode = myino;            
            dp->rec_len = BLKSIZE;        

            put_block(dev, bno, buf); 
            return 1;
        }
    }
}

int rm_child(MINODE *parent, char *name)
{
    DIR *dp, *prevdp, *lastdp;
    char *cp, *lastcp, buf[BLKSIZE], tmp[256], *startptr, *endptr;
    INODE *ip = &parent->INODE;

    for (int i = 0; i < 12; i++) 
    {
        if (ip->i_block[i] != 0)
        {
            get_block(parent->dev, ip->i_block[i], buf); 
            dp = (DIR *)buf;
            cp = buf;

            while (cp < buf + BLKSIZE)
            {
                strncpy(tmp, dp->name, dp->name_len);
                tmp[dp->name_len] = 0;               

                if (!strcmp(tmp, name)) 
                {
                    if (cp == buf && cp + dp->rec_len == buf + BLKSIZE) 
                    {
                        bdalloc(parent->dev, ip->i_block[i]);
                        ip->i_size -= BLKSIZE;

                        while (ip->i_block[i + 1] != 0 && i + 1 < 12) 
                        {
                            i++;
                            get_block(parent->dev, ip->i_block[i], buf);
                            put_block(parent->dev, ip->i_block[i - 1], buf);
                        }
                    }

                    else if (cp + dp->rec_len == buf + BLKSIZE) 
                    {
                        prevdp->rec_len += dp->rec_len;
                        put_block(parent->dev, ip->i_block[i], buf);
                    }

                    else 
                    {
                        lastdp = (DIR *)buf;
                        lastcp = buf;

                        while (lastcp + lastdp->rec_len < buf + BLKSIZE) 
                        {
                            lastcp += lastdp->rec_len;
                            lastdp = (DIR *)lastcp;
                        }

                        lastdp->rec_len += dp->rec_len;

                        startptr = cp + dp->rec_len; 
                        endptr = buf + BLKSIZE;      

                        memmove(cp, startptr, endptr - startptr); 
                        put_block(parent->dev, ip->i_block[i], buf);
                    }

                    parent->dirty = 1;
                    iput(parent);
                    return 0;
                }

                prevdp = dp;
                cp += dp->rec_len;
                dp = (DIR *)cp;
            }
        }
    }
    printf("ERROR: child not found\n");
    return -1;
}
