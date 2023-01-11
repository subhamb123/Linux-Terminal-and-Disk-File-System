int is_empty(MINODE *mip)
{
    char buf[BLKSIZE], *cp, temp[256];
    DIR *dp;
    INODE *ip = &mip->INODE;

    if (ip->i_links_count > 2) 
    {
        return 0;
    }
    else if (ip->i_links_count == 2) 
    {
        for (int i = 0; i < 12; i++) 
        {
            if (ip->i_block[i] == 0)
                break;
            get_block(mip->dev, mip->INODE.i_block[i], buf); 
            dp = (DIR *)buf;
            cp = buf;

            while (cp < buf + BLKSIZE) 
            {
                strncpy(temp, dp->name, dp->name_len);
                temp[dp->name_len] = 0;
                printf("%8d%8d%8u %s\n", dp->inode, dp->rec_len, dp->name_len, temp);
                if (strcmp(temp, ".") && strcmp(temp, ".."))                         
                {
                    return 0;
                }
                cp += dp->rec_len; 
                dp = (DIR *)cp;
            }
        }
    }
    return 1; 
}

int rmdir(char *pathname){
    int ino = getino(pathname); 
    MINODE *mip = iget(dev, ino); 

    if (mip->refCount > 2) {
        printf("minode is busy\n");
        return -1;
    }

    if (!is_empty(mip)) {
        printf("DIR isn't empty\n");
        return -1;
    }

    int pino = findino(mip, &ino); 
    MINODE *pmip = iget(mip->dev, pino);

    findmyname(pmip, ino, name); 

    rm_child(pmip, name);

    pmip->INODE.i_links_count--;
    pmip->dirty = 1;
    iput(pmip);

    bdalloc(mip->dev, mip->INODE.i_block[0]);
    idalloc(mip->dev, mip->ino);
    iput(mip);
}
