//
int kmkdir(MINODE *pip, char *name){
    //(4).1. Allocate an INODE and a disk block:
    int ino = ialloc(dev);
    int blk = balloc(dev);

    //(4).2. mip = iget(dev, ino) // load INODE into a minode
    MINODE *mip = iget(dev, ino);

    //initialize mip->INODE as a DIR INODE;
    INODE *ip = &mip->INODE;
    ip->i_mode = 0x41ED;         // OR 040755: DIR type and permissions
    ip->i_uid = running->uid;    // Owner uid
    ip->i_gid = running->gid;    // Group Id
    ip->i_size = BLKSIZE;        // Size in bytes
    ip->i_links_count = 2;       // Links count=2 because of . and ..
    ip->i_atime = time(0L);      // set to current time
    ip->i_ctime = time(0L);      // set to current time
    ip->i_mtime = time(0L);      // set to current time
    ip->i_blocks = 2;            // LINUX: Blocks count in 512-byte chunks
    ip->i_block[0] = blk;        // new DIR has one data block
    //end of creating an inode

    //printf("iNode = %d", ip->mode);
    mip->INODE.i_block[0] = blk;

    // clears all the block memeory
    for (int i = 1; i < 15; i++) 
        ip->i_block[i] = 0;

    mip->dirty = 1;
    iput(mip); // write INODE back to disk

    //(4).3. make data block 0 of INODE to contain . and .. entries;
    char buf[BLKSIZE], *cp;
    //get_block(dev, blk, buf); // get the newly allocated block
    DIR *dp = (DIR *)buf;

    cp = buf;

    // making . entry
    dp->inode = ino;   // child ino
    dp->rec_len = 12;  // 4 * [(8 + name_len + 3) / 4]
    dp->name_len = 1;  // len of name
    dp->name[0] = '.'; // name

    //Go to the end and find new open entry space
    cp += dp->rec_len; // advancing to end of '.' entry
    dp = (DIR *)cp;

    //making .. entry
    dp->inode = pip->ino;       // setting to parent ino
    //Use the remainder of the block we allocated
    dp->rec_len = BLKSIZE - 12; // size is rest of block
    
    //Set up name & length
    dp->name_len = 2;           // size of the name
    dp->name[0] = '.';
    dp->name[1] = '.';

    //write to disk block blk
    put_block(dev, blk, buf);

    //(4).4. Enters (ino, basename) as a dir_entry to the parent INODE;
    enter_name(pip, ino, name);
}

int mymkdir(char *pathname){
    //(1). divide pathname into dirname and basename, e.g. pathname=/a/b/c, then
    char pathcpy1[256], pathcpy2[256];
    strcpy(pathcpy1, pathname);
    strcpy(pathcpy2, pathname);
    char *parent = dirname(pathcpy1);
    char *child = basename(pathcpy2);

    //(2). // dirname must exist and is a DIR:
    //Find the parent inode so that we can verify the location exists
    int pino = getino(parent);
    MINODE *pmip = iget(dev, pino);

    // verifies parent is a dir
    if (!S_ISDIR(pmip->INODE.i_mode)) { 
        printf("Not a directory\n");
        return -1;
    }

    //(3). // basename must not exist in parent DIR:
    // use the serch fuc to see if it's already exits (child node)
    if (search(pmip, child) != 0){
        printf("Basename exists in parent\n");
        return -1;
    }

    //(4). call kmkdir(pmip, basename) to create a DIR;
    // if it doesnot exit them we will call kmdir to crear the directory 
    kmkdir(pmip, child);

    //(5). increment parent INODE’s links_count by 1 and mark pmip dirty;
    pmip->INODE.i_links_count++;
    pmip->dirty = 1;
    iput(pmip);
}
// the diff is that -
// size of block 
//links count 1 
// the block to 0
int kmkdir_C(MINODE *pip, char *name){
    //(4).1. Allocate an INODE and a disk block:
    int ino = ialloc(dev);
    int blk = balloc(dev);

    //(4).2. mip = iget(dev, ino) // load INODE into a minode
    MINODE *mip = iget(dev, ino);

    //initialize mip->INODE as a DIR INODE;
    INODE *ip = &mip->INODE;
    ip->i_mode = 33188;         // OR 040755: DIR type and permissions
    ip->i_uid = running->uid;    // Owner uid
    ip->i_gid = running->gid;    // Group Id
    ip->i_size = 0;        // Size in bytes
    ip->i_links_count = 2;       // Links count=2 because of . and ..
    ip->i_atime = time(0L);      // set to current time
    ip->i_ctime = time(0L);      // set to current time
    ip->i_mtime = time(0L);      // set to current time
    ip->i_blocks = 2;            // LINUX: Blocks count in 512-byte chunks
    ip->i_block[0] = blk;        // new DIR has one data block

    //printf("iNode = %d", ip->mode);

    // mip->INODE.i_block[0] = blk;
    // for (int i = 1; i < 15; i++) // clears all the block memeory
    //     ip->i_block[i] = 0;

    //Mark for updates and write to disk
    mip->dirty = 1;
    iput(mip); // write INODE back to disk

    //(4).3. make data block 0 of INODE to contain . and .. entries;
    // char buf[BLKSIZE], *cp;
    // //get_block(dev, blk, buf); // get the newly allocated block
    // DIR *dp = (DIR *)buf;
    // cp = buf;

    // // making . entry
    // dp->inode = ino;   // child ino
    // dp->rec_len = 12;  // 4 * [(8 + name_len + 3) / 4]
    // dp->name_len = 1;  // len of name
    // dp->name[0] = '.'; // name

    // cp += dp->rec_len; // advancing to end of '.' entry
    // dp = (DIR *)cp;

    // //making .. entry
    // dp->inode = pip->ino;       // setting to parent ino
    // dp->rec_len = BLKSIZE - 12; // size is rest of block
    // dp->name_len = 2;           // size of the name
    // dp->name[0] = '.';
    // dp->name[1] = '.';

    // //write to disk block blk
    // put_block(dev, blk, buf);

    //(4).4. Enters (ino, basename) as a dir_entry to the parent INODE;
    enter_name(pip, ino, name);
}

int mycreat(char *pathname){
    //(1). divide pathname into dirname and basename, e.g. pathname=/a/b/c, then
    char pathcpy1[256], pathcpy2[256];
    strcpy(pathcpy1, pathname);
    strcpy(pathcpy2, pathname);
    char *parent = dirname(pathcpy1);
    char *child = basename(pathcpy2);

    //(2). // dirname must exist and is a DIR:
    int pino = getino(parent);
    MINODE *pmip = iget(dev, pino);
    if (!S_ISDIR(pmip->INODE.i_mode)) { // verifies parent is a dir
        printf("Not a directory\n");
        return -1;
    }

    //(3). // basename must not exist in parent DIR:
    if (search(pmip, child) != 0){
        printf("Basename exists in parent\n");
        return -1;
    }

    //(4). call kmkdir(pmip, basename) to create a DIR;
    kmkdir_C(pmip, child);

    //(5). increment parent INODE’s links_count by 1 and mark pmip dirty;
    pmip->INODE.i_links_count++;
    pmip->dirty = 1;
    iput(pmip);
}

//Enters a new directory entry(child) in the parent directory
//enters a [ino, name] as a new dir_entry into a parent directory.
//first finds the spaced remaining and then it 
//Loops over the direct blocks and look for space the enter our entry
// at last entry
//thne Find the amount of space remaining
//If there is sufficient space for this entry continue to write the name 
//
int enter_name(MINODE *pip, int myino, char *myname)
{
    //pass in the 
    
    //Read in that block into a buffer and search for space for entry
    char buf[BLKSIZE]

    //Store our current position in the buffer
    char *cp;
    
    
    int bno;
    //allocating space for storing w
    INODE *ip;
    DIR *dp;

     //Find the amount of space remaining 
    int need_len = 4 * ((8 + strlen(myname) + 3) / 4); //ideal length of entry

    ip = &pip->INODE; // get the inode

     //Loop over the direct blocks and look for space the enter our entry
    for (int i = 0; i < 12; i++)
    {
         //If we are at a block that has no information
        if (ip->i_block[i] == 0)
        {
            break;
        }

        //Get the block number for the data stored
        bno = ip->i_block[i];

        //step1 - get parent's data block into buf
        get_block(pip->dev, ip->i_block[i], buf); // get the block
        //Create a dir entry pointer based on the information on the block
        dp = (DIR *)buf;
        //Store our current position in the buffer
        cp = buf;


         //Loop until we have gone through all of the entries on the block
        while (cp + dp->rec_len < buf + BLKSIZE) // Going to last entry of the block
        {
            printf("%s\n", dp->name);
            //Increase the current position by the entry length
            cp += dp->rec_len;
            //Recast to new spot
            dp = (DIR *)cp;
        }

        // at last entry
        int ideal_len = 4 * ((8 + dp->name_len + 3) / 4); // ideal len of the name
        
        //Find the amount of space remaining
        int remainder = dp->rec_len - ideal_len;          // remaining space

        //If there is sufficient space for this entry continue
        if (remainder >= need_len)
        {                            // space available for new netry

            //Set the record length up to be proper
            dp->rec_len = ideal_len; //trim current entry to ideal len

            //Go to the end and find new open entry space
            cp += dp->rec_len;       // advance to end
            dp = (DIR *)cp;          // point to new open entry space

            //Set up the entry with the ino, name, legnth of name, and the record size
            dp->inode = myino;             // add the inode
            strcpy(dp->name, myname);      // add the name
            dp->name_len = strlen(myname); // len of name
            dp->rec_len = remainder;       // size of the record

            //Write to disk
            put_block(dev, bno, buf); // save block
            return 0;
        }
        else
        {                         // not enough space in block
            
            //Handle when we need to allocate space for this entry
            //Set the size of this entry
            ip->i_size = BLKSIZE; // size is new block

            //Get a new block allocated and store it at this spot
            bno = balloc(dev);    // allocate new block
            ip->i_block[i] = bno; // add the block to the list


            //Since we have changed the ino mark it as dirty
            pip->dirty = 1;       // ino is changed so make dirty

            //Read the block in from memory
            get_block(dev, bno, buf); // get the blcok from memory
            //Cast our current position and entry to where we are at with this new block of data
            dp = (DIR *)buf;
            cp = buf;
            
            //Store the name length, inode, record length, and name
            dp->name_len = strlen(myname); // add name len
            strcpy(dp->name, myname);      // name
            dp->inode = myino;             // inode
            dp->rec_len = BLKSIZE;         // only entry so full size

             //Write to disk
            put_block(dev, bno, buf); //save
            
            //return success
            return 1;
        }
    }
}
