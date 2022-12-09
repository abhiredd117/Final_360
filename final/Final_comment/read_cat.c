int read_file() {
    int fd = 0, n_bytes = 0;
    printf("Enter a file descriptor: ");
    scanf("%d", &fd);
    printf("Enter the number of bytes to read: ");
    scanf("%d", &n_bytes);
    
    //checks if the file discriptoris valid 
    if (fd < 0 || fd > (NFD-1)) {

        printf("error: invalid provided fd\n");
        return -1;
    }

    // verify fd is open for RD or RW
    if (running->fd[fd]->mode != READ && running->fd[fd]->mode != READ_WRITE) {
        printf("error: provided fd is not open for read or write\n");
        return -1;
    }

    char buf[BLKSIZE];
    //call myread 
    return myread(fd, buf, n_bytes);
}

int myread(int fd, char *buf, int n_bytes) {
    OFT *oftp = running->fd[fd]; //open file table
    MINODE *mip = oftp->minodePtr;
    int count = 0, lbk, startByte, blk;
    int ibuf[BLKSIZE] = { 0 };
    int doubly_ibuf[BLKSIZE] = { 0 };

    int avil = mip->INODE.i_size - oftp->offset;
    char *cq = buf;

    if (n_bytes > avil)
        n_bytes = avil;

    //Loop while bytes is still positive & available is aswell
    while (n_bytes && avil) {
        //This is where we are starting to read from
        lbk = oftp->offset / BLKSIZE;
        //Adjust the start byte to read from
        startByte = oftp->offset % BLKSIZE;

        if (lbk < 12) {                             // direct blocks
            //Read from the direct blocks to start
            blk = mip->INODE.i_block[lbk];
        } else if (lbk >= 12 && lbk < 256 + 12) {   // indirect blocks
            // //Read the block numbers into the integerBuffer then find block
            get_block(mip->dev, mip->INODE.i_block[12], ibuf);
            //Subtract 12 from the lbk we are finding because that is the offset for the indirect section
            blk = ibuf[lbk - 12]; // offset of 12
        } else {                                    // doubly indirect blocks
            // read block from i_block[13]
            get_block(mip->dev, mip->INODE.i_block[13], (char *)ibuf);
            // use mailman's algorithm to reset blk to the correct doubly indirect block
            int chunk_size = (BLKSIZE / sizeof(int));

            //Find the proper block number based on how many pointers can be in a block
            lbk = lbk - chunk_size - 12; // reset lbk to 0 relatively
            blk = ibuf[lbk / chunk_size]; // divide 'addresses'/indices by 256
            get_block(mip->dev, blk, doubly_ibuf);
            // now modulus to get the correct mapping
            blk = doubly_ibuf[lbk % chunk_size];
        }

        char readbuf[BLKSIZE];
        // get data block into readbuf[BLKSIZE]
        get_block(mip->dev, blk, readbuf);

        // copy from startByte to buf[]
        char *cp = readbuf + startByte;
        int remainder = BLKSIZE - startByte;

        // copy entire BLKSIZE at a time
        if (n_bytes <= remainder) {
            //Adjust offsets
            memcpy(cq, cp, n_bytes);
            cq += n_bytes;
            //cp += n_bytes;
            count += n_bytes;
            oftp->offset += n_bytes;
            avil -= n_bytes;
            remainder -= n_bytes;
            n_bytes = 0;
        } else {
            memcpy(cq, cp, remainder);
            cq += remainder;
            //cp += remainder;
            count += remainder;
            oftp->offset += remainder;
            avil -= remainder;
            n_bytes -= remainder;
            remainder = 0;
        }
    }
    // printf("myread: read %d char from file descriptor %d, offset: %x\n", count, fd, oftp->offset); // FOR DEBUG
    return count;
}
//read the given file 
// takes the name and reads it 
int my_cat(char *filename) {

    printf("%s\n", filename);

    printf("CAT: running->cwd->ino, address: %d\t%x\n", running->cwd->ino, running->cwd);
    int n;
    //memory fr buffer 
    char mybuf[BLKSIZE];

    //Open the file to read
    int fd = open_file(filename, READ);

    //Confirm the fd is valid
    if (fd < 0 || fd > (NFD-1)) {
        printf("error, invalid fd to cat\n");
        close_file(fd);
        return -1;
    }
    //Loop through file and read it
    while (n = myread(fd, mybuf, BLKSIZE)) {

        mybuf[n] = 0;
        //Set the end of the buffer read to null
        char *cp = mybuf;
        //Loop through the buffer until we meet a null character
        while (*cp != '\0') {
            //Print newline if we need too or just print the character
            if (*cp == '\n') {
                printf("\n");
            } else {
                printf("%c", *cp);
            }
            //Increase the current position
            cp++;
        }
        //printf("%s", mybuf); // to be fixed
    }
    //Close the file & return
    close_file(fd);

    printf("END OF CAT: running->cwd->ino, address: %d\t%x\n", running->cwd->ino, running->cwd);
    return 0;
}
