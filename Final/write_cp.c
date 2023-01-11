int mywrite(int fd, char buf[ ], int nbytes)
{
    OFT *oftp = running->fd[fd];
    MINODE *mip = oftp->minodePtr;
    INODE *ip = &mip->INODE;
    int count = 0, lblk, startByte, lbk, remain, doubleblk;
    char ibuf[BLKSIZE] = { 0 }, doubly_ibuf[BLKSIZE] = { 0 };
    char *cq = buf;

    while (nbytes > 0 ){

        //compute LOGICAL BLOCK (lbk) and the startByte in that lbk:
        lbk       = oftp->offset / BLKSIZE;
        startByte = oftp->offset % BLKSIZE;

        // I only show how to write DIRECT data blocks, you figure out how to
        // write indirect and double-indirect blocks.

        if (lbk < 12){                         // direct block
            if (ip->i_block[lbk] == 0){   // if no data block yet

                mip->INODE.i_block[lbk] = balloc(mip->dev);// MUST ALLOCATE a block
            }
            lbk = mip->INODE.i_block[lbk];      // blk should be a disk block now
        }
        else if (lbk >= 12 && lbk < 256 + 12){ // INDIRECT blocks
            // HELP INFO:
            if (ip->i_block[12] == 0){
                //allocate a block for it;
                //zero out the block on disk !!!!
                int block_12 = ip->i_block[12] = balloc(mip->dev);

                if (block_12 == 0)
                    return 0;

                get_block(mip->dev, ip->i_block[12], ibuf);
                int *ptr = (int *)ibuf;
                for(int i = 0; i < (BLKSIZE / sizeof(int)); i++)
                {
                    ptr[i] = 0;
                }

                put_block(mip->dev, ip->i_block[12], ibuf);
                mip->INODE.i_blocks++;
            }
            //get i_block[12] into an int ibuf[256];
            int ibuf[BLKSIZE / sizeof(int)] = { 0 };
            get_block(mip->dev, ip->i_block[12], (char *)ibuf);

            lbk = ibuf[lbk - 12];
            if (lbk==0){
                //allocate a disk block;
                //record it in i_block[12];
                lbk = ibuf[lblk - 12] = balloc(mip->dev);
                ip->i_blocks++;
                put_block(mip->dev, ip->i_block[12], (char *)ibuf);
            }
        }
        else{
            // double indirect blocks */
            lblk = lblk - (BLKSIZE/sizeof(int)) - 12;
            //printf("%d\n", mip->INODE.i_block[13]);
            if(mip->INODE.i_block[13] == 0)
            {
                int block_13 = ip->i_block[13] = balloc(mip->dev);

                if (block_13 == 0)
                    return 0;

                get_block(mip->dev, ip->i_block[13], ibuf);
                int *ptr = (int *)ibuf;
                for(int i = 0; i < (BLKSIZE / sizeof(int)); i++){
                    ptr[i] = 0;
                }
                put_block(mip->dev, ip->i_block[13], ibuf);
                ip->i_blocks++;
            }
            int doublebuf[BLKSIZE/sizeof(int)] = {0};
            get_block(mip->dev, ip->i_block[13], (char *)doublebuf);
            doubleblk = doublebuf[lblk/(BLKSIZE / sizeof(int))];

            if(doubleblk == 0){
                doubleblk = doublebuf[lblk/(BLKSIZE / sizeof(int))] = balloc(mip->dev);
                if (doubleblk == 0)
                    return 0;
                get_block(mip->dev, doubleblk, doubly_ibuf);
                int *ptr = (int *)doubly_ibuf;
                for(int i = 0; i < (BLKSIZE / sizeof(int)); i++){
                    ptr[i] = 0;
                }
                put_block(mip->dev, doubleblk, doubly_ibuf);
                ip->i_blocks++;
                put_block(mip->dev, mip->INODE.i_block[13], (char *)doublebuf);
            }

            memset(doublebuf, 0, BLKSIZE / sizeof(int));
            get_block(mip->dev, doubleblk, (char *)doublebuf);
            if (doublebuf[lblk % (BLKSIZE / sizeof(int))] == 0) {
                lbk = doublebuf[lblk % (BLKSIZE / sizeof(int))] = balloc(mip->dev);
                if (lbk == 0)
                    return 0;
                ip->i_blocks++;
                put_block(mip->dev, doubleblk, (char *)doublebuf);
            }
        }

        /* all cases come to here : write to the data block */
        char wbuf[BLKSIZE] = { 0 };
        get_block(mip->dev, lbk, wbuf);   // read disk block into wbuf[ ]
        char *cp = wbuf + startByte;      // cp points at startByte in wbuf[]
        remain = BLKSIZE - startByte;     // number of BYTEs remain in this block

        while (remain > 0){               // write as much as remain allows
            *cp++ = *cq++;              // cq points at buf[ ]
            nbytes--; remain--;         // dec counts
            oftp->offset++;             // advance offset
            if (oftp->offset > mip->INODE.i_size)  // especially for RW|APPEND mode
                mip->INODE.i_size++;    // inc file size (if offset > fileSize)
            if (nbytes <= 0) break;     // if already nbytes, break
        }
        put_block(mip->dev, lbk, wbuf);   // write wbuf[ ] to disk

        // loop back to outer while to write more .... until nbytes are written
    }

    mip->dirty = 1;       // mark mip dirty for iput()
    printf("wrote %d char into file descriptor fd=%d\n", nbytes, fd);
    return nbytes;
}

int write_file(int fd, char *buf)
{
    //1. Preprations:
    //ask for a fd   and   a text string to write;
    //e.g.     write 1 1234567890;    write 1 abcdefghij

    //2. verify fd is indeed opened for WR or RW or APPEND mode
    if (fd < 0 || fd > (NFD-1)) {
        printf("error: invalid provided fd\n");
        return -1;
    }

    if (running->fd[fd]->mode != WRITE && running->fd[fd]->mode != READ_WRITE) {
        printf("error: provided fd is not open for write\n");
        return -1;
    }

    //3. copy the text string into a buf[] and get its length as nbytes.
    int nbytes = sizeof(buf);

    return mywrite(fd, buf, nbytes);
}

int mycp(char *src, char *dest){
    int n = 0;
    char mybuf[BLKSIZE] = {0};
    int fdsrc = open_file(src, READ);
    mycreat(dest);
    int fddest = open_file(dest, READ_WRITE);

    printf("fdsrc %d\n", fdsrc);
    printf("fddest %d\n", fddest);

    if (fdsrc == -1 || fddest == -1) {
        if (fddest == -1) close_file(fddest);
        if (fdsrc == -1) close_file(fdsrc);
        return -1;
    }

    memset(mybuf, '\0', BLKSIZE);
    while ( n = myread(fdsrc, mybuf, BLKSIZE)){
        mybuf[n] = 0;
        mywrite(fddest, mybuf, n);
        memset(mybuf, '\0', BLKSIZE);
    }
    close_file(fdsrc);
    close_file(fddest);
    return 0;
}
