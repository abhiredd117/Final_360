//creates a file ONLY called newFileName which has the SAME inode (number) as that of oldFileName.

int link(char *old, char *new){
    //(1). // verify old_file exists and is not a DIR;
    int oino = getino(old);
    MINODE *omip = iget(dev, oino);
    //check omip->INODE file type (must not be DIR).
    if (S_ISDIR(omip->INODE.i_mode)) { //Confirm we do not have a directory on our hands
        printf("is directory, not allowed\n");
        return -1;
    }

    //(2). // new_file must not exist yet:
    //Store the device we used for the old
    int nino = getino(new);
    if(nino != 0){
        printf("New file exists\n");
        return -1;
    }

    //(3). creat new_file with the same inode number of old_file:
    char *parent = dirname(new), *child = basename(new);
    int pino = getino(parent);
    MINODE *pmip = iget(dev, pino);
    // creat entry in new parent DIR with same inode number of old_file
    enter_name(pmip, oino, child);

    //(4).Increase the amount of links for the old 
    omip->INODE.i_links_count++; //Increase the amount of links for the old 
    omip->dirty = 1; // modifed so they are dirst 
    //Write to disk
    iput(omip);
    iput(pmip);
}
//nlinks & removes a file from our filesystem
int unlink(char *filename){
    //(1). get filenmae’s minode:
    int ino = getino(filename);
    MINODE *mip = iget(dev, ino);

    if (S_ISDIR(mip->INODE.i_mode)) {
        printf("dir cannot be link; cannot unlink %s\n", filename);
        return -1;
    }

    //(2). remove name entry from parent DIR’s data block:
    char *parent = dirname(filename), *child = basename(filename);
    int pino = getino(parent);
    MINODE *pmip = iget(dev, pino);
    //findmyname(pmip, ino, name); //find name from parent DIR
    rm_child(pmip, filename);
    pmip->dirty = 1;
    //
    iput(pmip);

    //(3). decrement INODE’s link_count by 1
    mip->INODE.i_links_count--;

    if (mip->INODE.i_links_count > 0)
        mip->dirty = 1; // for write INODE back to disk
    else { // if links_count = 0: remove filename
        /* deallocate its data blocks and inode */
        bdalloc(mip->dev, mip->INODE.i_block[0]);
        idalloc(mip->dev, mip->ino);
    }
    iput(mip); // release mip
}


