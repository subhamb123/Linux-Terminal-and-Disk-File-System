int symlink(char *old, char *new) {
    int old_ino = getino(old);
    MINODE *omip = iget(dev, old_ino);

    if (old_ino == -1) {
        printf("%s does not exist\n", old);
        return -1;
    }

    mycreat(new);

    int new_ino = getino(new);
     if (new_ino == -1) {
        printf("%s does not exist\n", new);
        return -1;
    }

    MINODE *nmip = iget(dev, new_ino);
    nmip->INODE.i_mode = 0xA1FF; 
    nmip->dirty = 1;

    strncpy(nmip->INODE.i_block, old, 84);

    nmip->INODE.i_size = strlen(old) + 1; 

    omip->dirty = 1;
    iput(omip);
}

int readlink(char *file){
    dev = file[0] == '/' ? root->dev : running->cwd->dev;

    int ino = getino(file);
    if(ino == -1){
        printf("Error: No file found %s", file);
        return -1;
    }

    MINODE *mip = iget(dev,ino);

    if(!S_ISLNK(mip->INODE.i_mode)){
        printf("Error: file is not a LNK file");
        return -1;
    }

    char buf[BLKSIZE];
    get_block(dev, mip->INODE.i_block[0], buf);

    dev = buf[0] == '/' ? root->dev : running->cwd->dev;

    int lnkino = getino(buf);
    if(lnkino == -1){
        printf("Error: File linked is invalid");
        return -1;
    }

    MINODE *lnkmip = iget(dev,lnkino);

    printf("File size: %d\n",lnkmip->INODE.i_size);
}
