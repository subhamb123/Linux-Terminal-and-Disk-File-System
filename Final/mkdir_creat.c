int kmkdir(MINODE *pip, char *name){
    int ino = ialloc(dev);
    int blk = balloc(dev);

    MINODE *mip = iget(dev, ino);

    INODE *ip = &mip->INODE;
    ip->i_mode = 0x41ED;         // OR 040755: DIR type and permissions
    ip->i_uid = running->uid;    // Owner uid
    ip->i_gid = running->gid;    // Group Id
    ip->i_size = BLKSIZE;        // Size in bytes
    ip->i_links_count = 2;       // Links count=2 because of . and ..
    ip->i_atime = time(0L);      // set to current time
    ip->i_ctime = time(0L);      // set to current time
    ip->i_mtime = time(0L);      // set to current time
    ip->i_blocks = 2;            // LINUX: Blocks count in 512-byte chunks
    ip->i_block[0] = blk;        // new DIR has one data block

    //printf("iNode = %d", ip->mode);

    mip->INODE.i_block[0] = blk;
    for (int i = 1; i < 15; i++) 
        ip->i_block[i] = 0;

    mip->dirty = 1;
    iput(mip); 

   
    char buf[BLKSIZE], *cp;
   
    DIR *dp = (DIR *)buf;
    cp = buf;

    
    dp->inode = ino;   
    dp->rec_len = 12; 
    dp->name_len = 1;  
    dp->name[0] = '.'; 

    cp += dp->rec_len; 
    dp = (DIR *)cp;

    
    dp->inode = pip->ino;       
    dp->rec_len = BLKSIZE - 12; 
    dp->name_len = 2;           
    dp->name[0] = '.';
    dp->name[1] = '.';

    put_block(dev, blk, buf);

    enter_name(pip, ino, name);
}

int mymkdir(char *pathname){
    char pathcpy1[256], pathcpy2[256];
    strcpy(pathcpy1, pathname);
    strcpy(pathcpy2, pathname);
    char *parent = dirname(pathcpy1);
    char *child = basename(pathcpy2);

    int pino = getino(parent);
    MINODE *pmip = iget(dev, pino);
    if (!S_ISDIR(pmip->INODE.i_mode)) { 
        printf("Not a directory\n");
        return -1;
    }

    if (search(pmip, child) != 0){
        printf("Basename exists in parent\n");
        return -1;
    }

    kmkdir(pmip, child);

    pmip->INODE.i_links_count++;
    pmip->dirty = 1;
    iput(pmip);
}
int kmkdir_C(MINODE *pip, char *name){
    int ino = ialloc(dev);
    int blk = balloc(dev);

    MINODE *mip = iget(dev, ino);

    INODE *ip = &mip->INODE;
    ip->i_mode = 33188;         // OR 040755: DIR type and permissions
    ip->i_uid = running->uid;    // Owner uid
    ip->i_gid = running->gid;    // Group Id
    ip->i_size = 0;        // Size in bytes
    ip->i_links_count = 2;       // Links count=2 because of . and ..
    ip->i_atime = time(0L);      // set to current time
    ip->i_ctime = time(0L);      // set to current time
    ip->i_mtime = time(0L);      // set to current time
    ip->i_blocks = 2;            // LINUX: Blocks count in 512-byte chunks
    ip->i_block[0] = blk;        // new DIR has one data block


    mip->dirty = 1;
    iput(mip); 

    enter_name(pip, ino, name);
}

int mycreat(char *pathname){
    char pathcpy1[256], pathcpy2[256];
    strcpy(pathcpy1, pathname);
    strcpy(pathcpy2, pathname);
    char *parent = dirname(pathcpy1);
    char *child = basename(pathcpy2);

    int pino = getino(parent);
    MINODE *pmip = iget(dev, pino);
    if (!S_ISDIR(pmip->INODE.i_mode)) { 
        printf("Not a directory\n");
        return -1;
    }

    if (search(pmip, child) != 0){
        printf("Basename exists in parent\n");
        return -1;
    }

    kmkdir_C(pmip, child);

    pmip->INODE.i_links_count++;
    pmip->dirty = 1;
    iput(pmip);
}
