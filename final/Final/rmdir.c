
int is_empty(MINODE *mip)
{
    char buf[BLKSIZE], *cp, temp[256];
    DIR *dp;
    INODE *ip = &mip->INODE;

    if (ip->i_links_count > 2) // make sure there aren't any dirs inside -- links count only looks at dirs, still could have files
    {
        return 0;
    }
    else if (ip->i_links_count == 2) // only the 2 dirs '.' & '..' -> check files
    {
        for (int i = 0; i < 12; i++) // Search DIR direct blocks only
        {
            if (ip->i_block[i] == 0)
                break;
            get_block(mip->dev, mip->INODE.i_block[i], buf); // read the blocks
            dp = (DIR *)buf;
            cp = buf;

            while (cp < buf + BLKSIZE) // while not at the end of the block
            {
                strncpy(temp, dp->name, dp->name_len);
                temp[dp->name_len] = 0;
                printf("%8d%8d%8u %s\n", dp->inode, dp->rec_len, dp->name_len, temp); // print the name of the files
                if (strcmp(temp, ".") && strcmp(temp, ".."))                          // if neither match, there's another file
                {
                    return 0; // fail
                }
                cp += dp->rec_len; // go to next entry in block
                dp = (DIR *)cp;
            }
        }
    }
    return 1; // is empty
}

int rmdir(char *pathname){
    //(1). get in-memory INODE of pathname:
    int ino = getino(pathname); // get the inode number from the name
    MINODE *mip = iget(dev, ino); // get the inode itself

    //(2). verify INODE is a DIR (by INODE.i_mode field);
    //minode is not BUSY (refCount = 1);
    if (mip->refCount > 2) {
        printf("minode is busy\n");
        return -1;
    }

    //verify DIR is empty (traverse data blocks for number of entries = 2);
    if (!is_empty(mip)) {
        printf("DIR isn't empty\n");
        return -1;
    }

    //(3). /* get parent’s ino and inode */
    int pino = findino(mip, &ino); //get pino from .. entry in INODE.i_block[0]
    MINODE *pmip = iget(mip->dev, pino);

    //(4). /* get name from parent DIR’s data block
    findmyname(pmip, ino, name); //find name from parent DIR

    //(5). remove name from parent directory */
    rm_child(pmip, name);

    //(6). dec parent links_count by 1; mark parent pimp dirty;
    pmip->INODE.i_links_count--;
    pmip->dirty = 1;
    iput(pmip);

    //(7). /* deallocate its data blocks and inode */
    bdalloc(mip->dev, mip->INODE.i_block[0]);
    idalloc(mip->dev, mip->ino);
    iput(mip);
}


int rm_child(MINODE *parent, char *name) {
    //Get the INODE for the parent
    INODE *ip = &parent->INODE;

    //Loop through all blocks of memory in the parent inode
    for (int i = 0; i < 12; i++){

        //If the block actually has data
        if (ip->i_block[i] != 0) {

            //Create buffer to read in block data too
            char buf[BLKSIZE];

            //Read in the data from the block into our buffer
            get_block(parent->dev, ip->i_block[i], buf); // get block from file

            //Set up the current entry and the current position in the buffer
            DIR *dp = (DIR *) buf;
            char *cp = buf;

            //Create values that store the previous and last entries
            DIR *prevdp;

            //Loop over the block until we hit the end
            while (cp < buf + BLKSIZE){

                //Create a temporary string to store the name of the entry
                char temp[256];

                //Copy the name of the string and make sure there is a null value at the end
                strncpy(temp, dp->name, dp->name_len);
                temp[dp->name_len] = 0;
                //If this is the name we are looking for
                if (!strcmp(temp, name)){
                    //Handle when we are at the first record in the buffer
                    if (cp == buf && cp + dp->rec_len == buf + BLKSIZE) // first/only record
                    {
                        //Deallocate the current blk because it will now be empty and we dont need it
                        bdealloc(parent->dev, ip->i_block[i]);

                        //Decrease the size of the inode by the block we just delete so 1 BLKSIZE
                        ip->i_size -= BLKSIZE;

                        // filling hole in the i_blocks since we deallocated this one
                        while (ip->i_block[i + 1] != 0 && i + 1 < 12){
                            //Increase to next block
                            i++;
                             //Move each block down 1 starting
                            get_block(parent->dev, ip->i_block[i], buf);
                            put_block(parent->dev, ip->i_block[i - 1], buf);
                        }
                    }
                    //We are currently viewing the last entry in the block so we just make the size of the item we are removing go into that and increase it
                    else if (cp + dp->rec_len == buf + BLKSIZE)
                    {
                        //Add our record length to the previous records length so we get absorbed
                        prevdp->rec_len += dp->rec_len;

                        //Write to disk
                        put_block(parent->dev, ip->i_block[i], buf);
                    }

                        //Handle when we are inbetween the first and last entry on the block
                    else{

                        //Store the last entry and the last entry in the file
                        DIR *lastdp = (DIR *) buf;
                        char *lastcp = buf;

                        //Find the last entry in the block
                        while (lastcp + lastdp->rec_len < buf + BLKSIZE)
                        {
                            //Move our variables to this position
                            lastcp += lastdp->rec_len;
                            lastdp = (DIR *) lastcp;
                        }

                        //Increase the amount of size available
                        lastdp->rec_len += dp->rec_len;

                        //Find the point of the starting entry and the end of this block
                        char *start = cp + dp->rec_len;
                        char *end = buf + BLKSIZE;

                        //Shift everything left so that we have free space properly at the end of the block
                        memmove(cp, start, end - start);

                        //Write to disk
                        put_block(parent->dev, ip->i_block[i], buf);
                    }

                    //Update the parent node
                    parent->dirty = 1;
                    iput(parent);
                    return 0;
                }

                //Store previous entry
                prevdp = dp;

                //Advance current position by entry length
                cp += dp->rec_len;

                //Cast to next entry
                dp = (DIR *) cp;
            }
        }
    }

    //Error out
    printf("ERROR: could not find child\n");
        return -1;
}