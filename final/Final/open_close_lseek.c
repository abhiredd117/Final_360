int truncate(MINODE *mip)
{
///    1. release mip->INODE's data blocks;
///    a file may have 12 direct blocks, 256 indirect blocks and 256*256
///    double indirect data blocks. release them all.
///    2. update INODE's time field
    char buf[BLKSIZE];
    INODE *ip = &mip->INODE;
    // 12 direct blocks
    for (int i = 0; i < 12; i++) {
        if (ip->i_block[i] == 0)
            break;
        // now deallocate block
        bdalloc(dev, ip->i_block[i]);
        ip->i_block[i] = 0;
    }
    // now worry about indirect blocks and doubly indirect blocks
    // (see pp. 762 in ULK for visualization of data blocks)
    // indirect blocks:
    if (ip->i_block[12] != NULL) {
        get_block(dev, ip->i_block[12], buf); // follow the ptr to the block
        int *ip_indirect = (int *)buf; // reference to indirect block via integer ptr
        int indirect_count = 0;
        while (indirect_count < BLKSIZE / sizeof(int)) { // split blksize into int sized chunks (4 bytes at a time)
            if (ip_indirect[indirect_count] == 0)
                break;
            // deallocate indirect block
            bdalloc(dev, ip_indirect[indirect_count]);
            ip_indirect[indirect_count] = 0;
            indirect_count++;
        }
        // now all indirect blocks have been dealt with, deallocate reference to indirect
        bdalloc(dev, ip->i_block[12]);
        ip->i_block[12] = 0;
    }

    // doubly indirect blocks (same code as above, different variables):
    if (ip->i_block[13] != NULL) {
        get_block(dev, ip->i_block[13], buf);
        int *ip_doubly_indirect = (int *)buf;
        int doubly_indirect_count = 0;
        while (doubly_indirect_count < BLKSIZE / sizeof(int)) {
            if (ip_doubly_indirect[doubly_indirect_count] == 0)
                break;
            // deallocate doubly indirect block
            bdalloc(dev, ip_doubly_indirect[doubly_indirect_count]);
            ip_doubly_indirect[doubly_indirect_count] = 0;
            doubly_indirect_count++;
        }
        bdalloc(dev, ip->i_block[13]);
        ip->i_block[13] = 0;
    }

    //3. set INODE's size to 0 and mark Minode[ ] dirty
    mip->INODE.i_blocks = 0;
    mip->INODE.i_size = 0;
    mip->dirty = 1;
    iput(mip);
}

int open_file(char *file, int mode)
{
    //1. ask for a pathname and mode to open:
    //You may use mode = 0|1|2|3 for R|W|RW|APPEND
    

    //2. get pathname's inumber, minode pointer:
    int ino = getino(file);
    MINODE *mip = iget(dev, ino);

    //3. check mip->INODE.i_mode to verify it's a REGULAR file and permission OK.
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

//    Check whether the file is ALREADY opened with INCOMPATIBLE mode:
//    If it's already opened for W, RW, APPEND : reject.
//            (that is, only multiple R are OK)
//    for (int i = 0; i < NFD; i++) {
//        if (running->fd[i] == NULL)
//            break;
//        if (running->fd[i]->minodePtr == mip) {
//            if (mode != 0) {
//                printf("error: already open with incompatible mode\n");
//                return -1;
//            }
//        }
//    }

    //4. allocate a FREE OpenFileTable (OFT) and fill in values:
    OFT *oftp = (OFT *)malloc(sizeof(OFT));

    oftp->mode = mode;      // mode = 0|1|2|3 for R|W|RW|APPEND
    oftp->refCount = 1;
    oftp->minodePtr = mip;  // point at the file's minode[]

    //5. Depending on the open mode 0|1|2|3, set the OFT's offset accordingly:
    switch(mode){
        case 0 : oftp->offset = 0;     // R: offset = 0
            break;
        case 1 : truncate(mip);        // W: truncate file to 0 size
            oftp->offset = 0;
            break;
        case 2 : oftp->offset = 0;     // RW: do NOT truncate file
            break;
        case 3 : oftp->offset =  mip->INODE.i_size;  // APPEND mode
            break;
        default: printf("invalid mode\n");
            return(-1);
    }

    //7. find the SMALLEST i in running PROC's fd[ ] such that fd[i] is NULL
    //Let running->fd[i] point at the OFT entry
    int returned_fd = -1;
    for (int i = 0; i < NFD; i++) {
        if (running->fd[i] == NULL) {
            running->fd[i] = oftp;
            returned_fd = i;
            break;
        }
    }

    //8. update INODE's time field
    //M time is modified time 
    //A time is accesd time 
    //for R: touch atime.
    //for W|RW|APPEND mode : touch atime and mtime
    //mark Minode[ ] dirty
    if (mode != 0) { // not read, mtime
        mip->INODE.i_mtime = time(NULL);
    }
    mip->INODE.i_atime = time(NULL);
    mip->dirty = 1;
    iput(mip);

    //9. return i as the file descriptor
    return returned_fd;
}

//NOTE that mip = iget(dev, ino) in 2 is NOT iput().
//It will be iput() when the file descriptor is closed

int close_file(int fd)
{
//    1. verify fd is within range.
    if (fd < 0 || fd > (NFD-1)) {
        printf("error: fd not in range\n");
        return -1;
    }

//    2. verify running->fd[fd] is pointing at a OFT entry
    if (running->fd[fd] == NULL) {
        printf("error: not OFT entry\n");
        return -1;
    }

//    3. The following code segments should be fairly obvious:
    OFT *oftp = running->fd[fd];
    running->fd[fd] = 0;
    oftp->refCount--;
    if (oftp->refCount > 0)
        return 0;

//    last user of this OFT entry ==> dispose of the Minode[]
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

    // check if pointing at OFT entry
    if (running->fd[fd] == NULL) {
        printf("error: not OFT entry\n");
        return -1;
    }

    //From fd, find the OFT entry.
    OFT *oftp = running->fd[fd];
    if (position > oftp->minodePtr->INODE.i_size || position < 0) {
        printf("error: file size overrun\n");
    }

    //change OFT entry's offset to position but make sure NOT to over run either end of the file.
    int original_offset = oftp->offset;
    oftp->offset = position;

    //return originalPosition
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