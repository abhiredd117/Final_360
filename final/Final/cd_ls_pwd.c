/************* cd_ls_pwd.c file **************/

// It changes your working directory and takes the command to  go to a different directories 
//It does this by check if my node is a directory or not
//if it;s a pathname then it a 2 step thing and 
//first it gets i number based on the path name 
// then we use the i number ot find the mip.
int cd()
{

  //printf("cd: under construction READ textbook!!!!\n");

  //Get the ino
  int ino = getino(pathname);
  //Inode:  a data structure used to represent a file

  //Get MINODE (memory of the I node)
  MINODE *mip = iget(dev, ino);
  //Minode:  contains the in-memory INODE of the file 
  
  //check if my node is a directory or not 
  if(!S_ISDIR(mip->INODE.i_mode))
  {
   return 0;
  }
  // if it is a direnctory then  // cwd - cur
  //iput is writing to the file
  // running is the processs(it;s a glbal variable that tracks all the functiions)
  iput(running->cwd); // iput(): release a minode
  //once we got this 
  
  running->cwd = mip; //change the directory 
  

  // READ Chapter 11.7.3 HOW TO chdir
}
//prints out all the files 
int ls_file(MINODE *mip, char *name)
{

     //Create strings to use while printing out the permissions for
    char *t1 = "xwrxwrxwr-------";
    char *t2 = "----------------";

    // creats string to srote file name
    char ftime[256];
    
    //Get what type of file/node we are looking at
    u16 mode = mip->INODE.i_mode;
    

    if (S_ISREG(mode)) 
        printf("%c", '-');
    else if (S_ISDIR(mode))
        printf("%c", 'd');
    else if (S_ISLNK(mode))
        printf("%c", 'l');
        else{
            printf("unrecoginzed value %d", mode);
        }
    for (int i = 8; i >= 0; i--)
    {
        if (mode & (1 << i))
            printf("%c", t1[i]); // print r|w|x printf("%c", t1[i]);
        else
            printf("%c", t2[i]); // or print -
    }

    printf("%4d ", mip->INODE.i_links_count); // link count
    printf("%4d ", mip->INODE.i_gid);         // gid
    printf("%4d ", mip->INODE.i_uid);         // uid
    printf("%8d ", mip->INODE.i_size);       // file size

    // print time in calendar form ftime[strlen(ftime)-1] = 0; // kill \n at end
    strcpy(ftime, ctime((time_t *)&(mip->INODE.i_mtime))); 

    // removes the \n
    ftime[strlen(ftime) - 1] = 0;    

    // prints the time       
    printf("%s ", ftime);                        
    
     //Print file name
    printf("%s", name);
    //Print the linked name if we need to
    if (S_ISLNK(mode))
    {
        printf(" -> %s", (char *)mip->INODE.i_block); // print linked name 
    }

    
   // we wrote this to test
    //printf("[%d %d]", mip->ino, mip->dev);
    //Print newline
    printf("\n");

    return 0;

}

int ls_dir(MINODE *mip) //print out the summary of the current directory 
{
    //Create strings to use while printing out the permissions for
  char *t1 = "xwrxwrxwr-------";
  char *t2 = "----------------";

  char ftime[256];

  MINODE *temp_mip;

  
  //u16 mode = mip->INODE.i_mode;

  char buf[BLKSIZE], temp[BLKSIZE];
// accese the certian attributed of the buff 
  DIR *dp;

  char *cp;

// Assume DIR has only one data block i_block[0]
//get_block - gets the block of the data and reads it into a buffer
  get_block(dev, mip->INODE.i_block[0], buf); 
  //acceseeign the attributes 
  dp = (DIR *)buf;

  //point at the location in the memory where we are at. 
  cp = buf;
  //keep on running 'cp' until the end of buffer.
  while (cp < buf + BLKSIZE)
  {
      //copying it to temp 
      strncpy(temp, dp->name, dp->name_len);
      //assinget the null char that it dosent come wjtn
      temp[dp->name_len] = 0;
      
      //gets the node we are going to display
      temp_mip = iget(dev, dp->inode);
      //display node 
      ls_file(temp_mip, temp);
      //set the dirty &re-insert
      temp_mip->dirty = 1;
      //
      iput(temp_mip); //----------------------- FIXME: this causes the loop

        //printf("[%d %s]  ", dp->inode, temp); // print [inode# name]

        // cp is itrator of the directory 
      cp += dp->rec_len;
      dp = (DIR *)cp;
  }
  printf("\n");
  return 0;
  
}

// prints the stuff in the directory 
int ls(){
    //
    int mode;
    printf("ls %s\n", pathname);
    // just printing out the contents of the working directory

    // chack if the pathname is null 
    if (!strcmp(pathname, ""))
    {
        ls_dir(running->cwd);
    }
    // where to ls to , in which diretory to ls into
    else
    {
        int ino = getino(pathname);
        //if it cannot find the diretory then it returns -1
        if (ino == -1)
        {
            printf("inode DNE\n");
            return -1;
        }
        //if it is found
        else
        {
            
            dev = root->dev;
            // get the I number, then we get the iNode 
            MINODE *mip = iget(dev, ino);
            
            mode = mip->INODE.i_mode;
            //checks if it's a directory or file 
            //"S_ISDIR" code to check if it is the directory.
            if (S_ISDIR(mode))
            {
                //pritns out the directory information
                ls_dir(mip);
            }
            else
            {
                //print out file information
                ls_file(mip, basename(pathname)); 

            }
            //re-establish node
            iput(mip); //-----------FIXME: this also causes the loop
        }
    }
    return 0;
    
 }


void rpwd(MINODE *wd){
    //Used later when we are finding the parent ino we store the current here
    int ino;    

    
    if (wd == root)
    {
        return;
    }
    //Create buffer we use when we lseek & read
    char buf[BLKSIZE], lname[256];
    //Get memory block 
    //reading form the disk into an arrray 
    get_block(dev, wd->INODE.i_block[0], buf);

    //printf("stage 1 \n");

    //Store the parent ino for later so we can retrieve the parent minode
    int parent_ino = findino(wd, &ino);
    
    //Get the parent node
    MINODE *pip = iget(dev, parent_ino);
    //Store the name of the current node
    // go through each i block and 
    findmyname(pip, ino, lname);
    //printf("stage 2 \n");
    
    //Call the same thing on the parent so we end up printing from the top down
    rpwd(pip);
    //printf("stage 3 \n");

    //Set the nparent node to dirty
    pip->dirty = 1;

    //Write node back 2 disk
    iput(pip);
    

    printf("/%s", lname);
    return;

}

char *pwd(MINODE *wd)
{
    //jstu print out /
    if (wd == root)
    {
        printf("/\n");
    }
    // if it's the root
    else
    {
        rpwd(wd);
        printf("\n");
    }
    
}
