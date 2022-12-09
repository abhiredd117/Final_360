int mywrite(int fd, char* buffer, int byteCount){
    //Get the OFT for the file we want too write too
    OFT *oftp = running->fd[fd];

    //Get the memory inode from the open file
    MINODE *mip = oftp->minodePtr;

    //Get the literal inode
    INODE *ip = &mip->INODE;

    //Buffers we store the integers for blocks in when reading from the blocks
    char ibuf[BLKSIZE] = { 0 };
    char dibuf[BLKSIZE] = { 0 };

    //Starting point for copying the buffer
    char *cq = buffer;

    while (byteCount > 0){
        printf("Bytes left: %d\n",byteCount);

        //Find the location of the lblk based on offset
        int lblk = oftp->offset / BLKSIZE;

        //Find where we are starting to write from
        int startByte = oftp->offset % BLKSIZE;

        //Create variable to store where we need to actually write too
        int blk;

        //Handle writing the direct blocks
        if (lblk < 12){
            //Make sure that we have somewhere to write too
            if(ip->i_block[lblk] == 0)ip->i_block[lblk] = balloc(mip->dev);

            //Set the blk up so we can write
            blk = ip->i_block[lblk];
        }

        //Handle writing the indirect blocks
        if(lblk >= 12 && lblk < 268){
            printf("Writing in single indirect now\n");
            //Make sure we have a place to write too
            if(ip->i_block[12] == 0){
                //Allocate space
                ip->i_block[12] = balloc(mip->dev);

                //Confirm the space is there if not exit
                printf("Checking for space\n");
                if(ip->i_block[12] == 0)return 0;
                printf("Created space single indriect\n");
                //Get the block that we just allocated's information into a buffer
                get_block(mip->dev, ip->i_block[12], ibuf);

                //Go to each section in the list of blocks stored and set them to 0
                int *ptr = (int *)ibuf;

                //Loop through the chunks and set them to null
                for(int i = 0; i < (BLKSIZE / sizeof(int)); i++){
                    ptr[i] = 0;
                }

                //Write to disk
                put_block(mip->dev, ip->i_block[12], ibuf);

                //Increase the # of blocks
                mip->INODE.i_blocks++;
            }

            //This is where we store the indirect pointers to blocks
            int indir_buf[BLKSIZE / sizeof(int)] = { 0 };

            //Read the information in
            get_block(mip->dev, ip->i_block[12], (char *)indir_buf);

            //Set the block we are using. adjust by the offste of 12 since we discard the first 12 direct blocks
            blk = indir_buf[lblk - 12];

            //If the blk is not around
            if(blk == 0){
                //Allocate new block and set its location
                indir_buf[lblk - 12] = balloc(mip->dev);

                //Set up blk so we can use it
                blk = indir_buf[lblk - 12];

                //Increase the amount of blocks
                ip->i_blocks++;

                //Write this update to the block on top
                put_block(mip->dev, ip->i_block[12], (char *)indir_buf);
            }
        }

        //Handle writing the doubly indirect blocks
        if(lblk >= 268){
            printf("Writing in double indirect now\n");
            //Adjust the lblk based on the the size of each chunk & the offset of the direct blocks
            lblk = lblk - (BLKSIZE/sizeof(int)) - 12;

            //Make sure we have a block to write too
            if(mip->INODE.i_block[13] == 0){

                //Allocate block
                ip->i_block[13] = balloc(mip->dev);

                //Confirm the allocation worked
                if (ip->i_block[13] == 0)return 0;

                //Get the data in the block we just allocated
                get_block(mip->dev, ip->i_block[13], ibuf);
                int *ptr = (int *)ibuf;

                //Make sure that all the blocks are null
                for(int i = 0; i < (BLKSIZE / sizeof(int)); i++){
                    ptr[i] = 0;
                }

                //Write the updates to the block
                put_block(mip->dev, ip->i_block[13], ibuf);

                //Increase the amount of blocks
                ip->i_blocks++;
            }

            //This is where we have we have the first set of indirect's
            int doublebuf[BLKSIZE/sizeof(int)] = {0};
            get_block(mip->dev, ip->i_block[13], (char *)doublebuf);

            //Determine the outside blk based on the blk passed in divided by the size of chunks
            int doubleblk = doublebuf[lblk/(BLKSIZE / sizeof(int))];

            //Make sure we have an outside blk
            if(doubleblk == 0){
                //Allocate a block
                doublebuf[lblk/(BLKSIZE / sizeof(int))] = balloc(mip->dev);

                //Store newly allocated block
                doubleblk = doublebuf[lblk/(BLKSIZE / sizeof(int))];

                //Confirm we successfully allocated the block
                if (doubleblk == 0)return 0;

                //Get the block for what we just allocated
                get_block(mip->dev, doubleblk, dibuf);
                int *ptr = (int *)dibuf;

                //Set all of the chunks to null
                for(int i = 0; i < (BLKSIZE / sizeof(int)); i++){
                    ptr[i] = 0;
                }

                //Write the updates to the inside blk
                put_block(mip->dev, doubleblk, dibuf);

                //Increase amount of blocks
                ip->i_blocks++;

                //Write the changes to the outside blk
                put_block(mip->dev, mip->INODE.i_block[13], (char *)doublebuf);
            }

            //Set the entire section of blks we are looking @ to 0
            memset(doublebuf, 0, BLKSIZE / sizeof(int));

            //Get the lowest most set of blocks
            get_block(mip->dev, doubleblk, (char *)doublebuf);

            //Make sure the lowest block is available
            if (doublebuf[lblk % (BLKSIZE / sizeof(int))] == 0) {
                //Allocate the lowest block
                doublebuf[lblk % (BLKSIZE / sizeof(int))] = balloc(mip->dev);

                //Assign the blk and use modulous to find the correct blk
                blk = doublebuf[lblk % (BLKSIZE / sizeof(int))];

                //Confirm we successfully allocated
                if (blk == 0)return 0;

                //Increase amount of blocks
                ip->i_blocks++;

                //Write the updates to blk
                put_block(mip->dev, doubleblk, (char *)doublebuf);
            }
        }

        //Create buffer & read in the blk we are looking at to write too
        char buf[BLKSIZE] = {0 };
        get_block(mip->dev, blk, buf);

        printf("We are writing to blk %d\n",blk);

        //Store the current position & how many blocks are remaining
        char *cp = buf + startByte;
        int remainder = BLKSIZE - startByte;

        //How much we need to adjust by
        int amt = remainder <= byteCount ? remainder : byteCount;

        //Adjust the info for the next loop
        memcpy(cp,cq, amt);
        cq += amt;
        oftp->offset += amt;
        byteCount -= amt;

        //if the file offset is greater than the size of the INODE swap the sizes
        if(oftp->offset > mip->INODE.i_size)mip->INODE.i_size = oftp->offset;

        //Write to disk
        put_block(mip->dev, blk, buf);
    }

    //Mark as dirty
    mip->dirty = 1;

    //Bytes written
    return byteCount;
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
//cpoies from one file to another 
int mycp(char *src, char *dest){
    int n = 0;
    char mybuf[BLKSIZE] = {0};
    //Open the two files
    int fdsrc = open_file(src, READ);
    //creat the destination file 
    mycreat(dest);
    //open it up for read&write 
    int fddest = open_file(dest, READ_WRITE);

    //print out the file disrupters are valid
    printf("fdsrc %d\n", fdsrc);
    printf("fddest %d\n", fddest);

    //check if the file discripters are valid 
    //Make sure we got two files back
    if (fdsrc == -1 || fddest == -1) {

        if (fddest == -1) close_file(fddest);
        if (fdsrc == -1) close_file(fdsrc);
        return -1;
    }

    //Make sure we are full of \0's
    memset(mybuf, '\0', BLKSIZE);

    //Loop while we can read
    while ( n = myread(fdsrc, mybuf, BLKSIZE)){
        mybuf[n] = 0;
        //Write
        mywrite(fddest, mybuf, n);
        //Reset the buffer
        memset(mybuf, '\0', BLKSIZE);
    }
    //Close the files & return
    close_file(fdsrc);
    close_file(fddest);
    return 0;
}