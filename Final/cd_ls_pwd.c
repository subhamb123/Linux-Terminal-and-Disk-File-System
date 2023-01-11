/************* cd_ls_pwd.c file **************/
int cd()
{
  //printf("cd: under construction READ textbook!!!!\n");
  int ino = getino(pathname);
  MINODE *mip = iget(dev, ino);
  if(!S_ISDIR(mip->INODE.i_mode))
  {
   return 0;

  }
  
  iput(running->cwd);
  running->cwd = mip;
  

  // READ Chapter 11.7.3 HOW TO chdir
}

int ls_file(MINODE *mip, char *name)
{
    char *t1 = "xwrxwrxwr-------";
    char *t2 = "----------------";

    char ftime[256];
    u16 mode = mip->INODE.i_mode;

    if (S_ISREG(mode)) 
        printf("%c", '-');
    else if (S_ISDIR(mode))
        printf("%c", 'd');
    else if (S_ISLNK(mode))
        printf("%c", 'l');
        else{
            printf("unrecoginzed value %d", mode);
        }
    for (int i = 8; i >= 0; i--)
    {
        if (mode & (1 << i))
            printf("%c", t1[i]); 
        else
            printf("%c", t2[i]); 
    }
    printf("%4d ", mip->INODE.i_links_count); 
    printf("%4d ", mip->INODE.i_gid);         
    printf("%4d ", mip->INODE.i_uid);         
    printf("%8d ", mip->INODE.i_size);       

    strcpy(ftime, ctime((time_t *)&(mip->INODE.i_mtime))); 
    ftime[strlen(ftime) - 1] = 0;                
    printf("%s ", ftime);                        

    printf("%s", name);
    if (S_ISLNK(mode))
    {
        printf(" -> %s", (char *)mip->INODE.i_block); 
    }

    
   
    printf("[%d %d]", mip->ino, mip->dev);
    printf("\n");

    return 0;

}

int ls_dir(MINODE *mip)
{
  char *t1 = "xwrxwrxwr-------";
  char *t2 = "----------------";

  char ftime[256];
  MINODE *temp_mip;

  u16 mode = mip->INODE.i_mode;

  char buf[BLKSIZE], temp[BLKSIZE];
  DIR *dp;
  char *cp;

  
  get_block(dev, mip->INODE.i_block[0], buf);
  dp = (DIR *)buf;
  cp = buf;

  while (cp < buf + BLKSIZE)
  {
      strncpy(temp, dp->name, dp->name_len);
      temp[dp->name_len] = 0;

      temp_mip = iget(dev, dp->inode);
      ls_file(temp_mip, temp);
      temp_mip->dirty = 1;
      iput(temp_mip); //----------------------- FIXME: this causes the loop

        //printf("[%d %s]  ", dp->inode, temp); // print [inode# name]

      cp += dp->rec_len;
      dp = (DIR *)cp;
  }
  printf("\n");
  return 0;
  
}

int ls(){
       int mode;
    printf("ls %s\n", pathname);
    if (!strcmp(pathname, ""))
    {
        ls_dir(running->cwd);
    }
    else
    {
        int ino = getino(pathname);
        if (ino == -1)
        {
            printf("inode DNE\n");
            return -1;
        }
        else
        {
            dev = root->dev;
            MINODE *mip = iget(dev, ino);
            mode = mip->INODE.i_mode;
            if (S_ISDIR(mode))
            {
                ls_dir(mip);
            }
            else
            {
                ls_file(mip, basename(pathname));
            }
            iput(mip); //-----------FIXME: this also causes the loop
        }
    }
    return 0;
    
 }


void rpwd(MINODE *wd){
    
    int ino;    

    //
    if (wd == root)
    {
        return;
    }
    
    char buf[BLKSIZE], lname[256];
    
    get_block(dev, wd->INODE.i_block[0], buf);

    //printf("stage 1 \n");

   
    int parent_ino = findino(wd, &ino);
    
    
    MINODE *pip = iget(dev, parent_ino);
    
    findmyname(pip, ino, lname);
    //printf("stage 2 \n");
    
    
    rpwd(pip);
    //printf("stage 3 \n");

    
    pip->dirty = 1;

    
    iput(pip);
    

    printf("/%s", lname);
    return;

}

char *pwd(MINODE *wd)
{
    if (wd == root)
    {
        printf("/\n");
    }
    else
    {
        rpwd(wd);
        printf("\n");
    }
    
}
