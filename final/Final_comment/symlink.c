//Links directories and files not on the same device too
//
int symlink(char *old, char *new) {
    //verify old exists (either dir or regular)
    int old_ino = getino(old);
    MINODE *omip = iget(dev, old_ino);

    if (old_ino == -1) {
        printf("%s does not exist\n", old);
        return -1;
    }
    mycreat(new);
    // set type of new to LNK (0xA000)
    int new_ino = getino(new);
     if (new_ino == -1) {
        printf("%s does not exist\n", new);
        return -1;
    }

    MINODE *nmip = iget(dev, new_ino);
    nmip->INODE.i_mode = 0xA1FF; // A1FF sets link perm bits correctly (rwx for all users)
    nmip->dirty = 1;

    // write the string old into the i_block[ ], which has room for 60 chars.
    // i_block[] + 24 after = 84 total for old
    strncpy(nmip->INODE.i_block, old, 84);

    // set /x/y/z file size = number of chars in oldName
    nmip->INODE.i_size = strlen(old) + 1; // +1 for '\0'

    // write the INODE of /x/y/z back to disk.
    omip->dirty = 1;
    iput(omip);
}



//get the block and take the dir of the buffer that u took in from the block.
int readlink(char *file){
    //Properly set up the device
    dev = file[0] == '/' ? root->dev : running->cwd->dev;

    //Get the ino so we can confirm if the file exists & is a LNK file
    int ino = getino(file);
    if(ino == -1){
        printf("Error: No file found %s", file);
        return -1;
    }

    //Get the memory inode for the file in question
    MINODE *mip = iget(dev,ino);

    //Confirm it is a LNK file
    if(!S_ISLNK(mip->INODE.i_mode)){
        printf("Error: file is not a LNK file");
        return -1;
    }

    //Buffer to copy the name in the LNK file too
    char buf[BLKSIZE];
    get_block(dev, mip->INODE.i_block[0], buf);

    //Find the file from the name now
    dev = buf[0] == '/' ? root->dev : running->cwd->dev;

    //Get the info so we can confirm the found name is a valid file
    int lnkino = getino(buf);
    if(lnkino == -1){
        printf("Error: File linked is invalid");
        return -1;
    }

    //Get memory inode for file found
    MINODE *lnkmip = iget(dev,lnkino);

    //Print
    printf("File size: %d\n",lnkmip->INODE.i_size);
}