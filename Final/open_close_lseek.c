int truncate(MINODE *mip)
{
    char buf[BLKSIZE];
    INODE *ip = &mip->INODE;
    for (int i = 0; i < 12; i++) {
        if (ip->i_block[i] == 0)
            break;
        bdalloc(dev, ip->i_block[i]);
        ip->i_block[i] = 0;
    }
    if (ip->i_block[12] != NULL) {
        get_block(dev, ip->i_block[12], buf); 
        int *ip_indirect = (int *)buf; 
        int indirect_count = 0;
        while (indirect_count < BLKSIZE / sizeof(int)) { 
            if (ip_indirect[indirect_count] == 0)
                break;
           
            bdalloc(dev, ip_indirect[indirect_count]);
            ip_indirect[indirect_count] = 0;
            indirect_count++;
        }
        
        bdalloc(dev, ip->i_block[12]);
        ip->i_block[12] = 0;
    }

    if (ip->i_block[13] != NULL) {
        get_block(dev, ip->i_block[13], buf);
        int *ip_doubly_indirect = (int *)buf;
        int doubly_indirect_count = 0;
        while (doubly_indirect_count < BLKSIZE / sizeof(int)) {
            if (ip_doubly_indirect[doubly_indirect_count] == 0)
                break;
                     
            bdalloc(dev, ip_doubly_indirect[doubly_indirect_count]);
            ip_doubly_indirect[doubly_indirect_count] = 0;
            doubly_indirect_count++;
        }
        bdalloc(dev, ip->i_block[13]);
        ip->i_block[13] = 0;
    }

    mip->INODE.i_blocks = 0;
    mip->INODE.i_size = 0;
    mip->dirty = 1;
    iput(mip);
}

int open_file(char *file, int mode)
{
    
    int ino = getino(file);
    MINODE *mip = iget(dev, ino);

    if (!S_ISREG(mip->INODE.i_mode)) {
        printf("error: not a regular file\n");
        return -1;
    }

    if (!(mip->INODE.i_uid == running->uid || running->uid)) {
        printf("permissions error: uid\n");
        return -1;
    }

    if (!(mip->INODE.i_gid == running->gid || running->gid)) {
        printf("permissions error: gid\n");
        return -(file, mode);
    }

    OFT *oftp = (OFT *)malloc(sizeof(OFT));

    oftp->mode = mode;      
    oftp->refCount = 1;
    oftp->minodePtr = mip;  

    switch(mode){
        case 0 : oftp->offset = 0;     
            break;
        case 1 : truncate(mip);        
            oftp->offset = 0;
            break;
        case 2 : oftp->offset = 0;     
            break;
        case 3 : oftp->offset =  mip->INODE.i_size;  
            break;
        default: printf("invalid mode\n");
            return(-1);
    }

    int returned_fd = -1;
    for (int i = 0; i < NFD; i++) {
        if (running->fd[i] == NULL) {
            running->fd[i] = oftp;
            returned_fd = i;
            break;
        }
    }

    if (mode != 0) { 
        mip->INODE.i_mtime = time(NULL);
    }
    mip->INODE.i_atime = time(NULL);
    mip->dirty = 1;
    iput(mip);

    return returned_fd;
}


int close_file(int fd)
{
    if (fd < 0 || fd > (NFD-1)) {
        printf("error: fd not in range\n");
        return -1;
    }

    if (running->fd[fd] == NULL) {
        printf("error: not OFT entry\n");
        return -1;
    }

    OFT *oftp = running->fd[fd];
    running->fd[fd] = 0;
    oftp->refCount--;
    if (oftp->refCount > 0)
        return 0;

    MINODE *mip = oftp->minodePtr;
    iput(mip);

    return 0;
}

int mylseek(int fd, int position)
{
    if (fd < 0 || fd > (NFD-1)) {
        printf("error: fd not in range\n");
        return -1;
    }

    if (running->fd[fd] == NULL) {
        printf("error: not OFT entry\n");
        return -1;
    }

    OFT *oftp = running->fd[fd];
    if (position > oftp->minodePtr->INODE.i_size || position < 0) {
        printf("error: file size overrun\n");
    }

    int original_offset = oftp->offset;
    oftp->offset = position;

    return original_offset;
}

int pfd() {
    printf("fd\tmode\toffset\tINODE [dev, ino]\n");
    for (int i = 0; i < NFD; i++) {
        if (running->fd[i] == NULL)
            break;
        printf("%d\t%s\t%d\t[%d, %d]\n", i, running->fd[i]->mode, running->fd[i]->offset, running->fd[i]->minodePtr->dev, running->fd[i]->minodePtr->ino);
    }
}
