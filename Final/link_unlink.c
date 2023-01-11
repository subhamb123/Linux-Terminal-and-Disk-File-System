int link(char *old, char *new){
   
    int oino = getino(old);
    MINODE *omip = iget(dev, oino);
    
    if (S_ISDIR(omip->INODE.i_mode)) {
        printf("is directory, not allowed\n");
        return -1;
    }

    int nino = getino(new);
    if(nino != 0){
        printf("New file exists\n");
        return -1;
    }

    char *parent = dirname(new), *child = basename(new);
    int pino = getino(parent);
    MINODE *pmip = iget(dev, pino);
    
    enter_name(pmip, oino, child);

    
    omip->INODE.i_links_count++; 
    omip->dirty = 1; 
    iput(omip);
    iput(pmip);
}

int unlink(char *filename){
    
    int ino = getino(filename);
    MINODE *mip = iget(dev, ino);

    if (S_ISDIR(mip->INODE.i_mode)) {
        printf("dir cannot be link; cannot unlink %s\n", filename);
        return -1;
    }
    
    char *parent = dirname(filename), *child = basename(filename);
    int pino = getino(parent);
    MINODE *pmip = iget(dev, pino);
    
    rm_child(pmip, filename);
    pmip->dirty = 1;
    iput(pmip);

    
    mip->INODE.i_links_count--;

    if (mip->INODE.i_links_count > 0)
        mip->dirty = 1; 
    else { 
        
        bdalloc(mip->dev, mip->INODE.i_block[0]);
        idalloc(mip->dev, mip->ino);
    }
    iput(mip); 
}


