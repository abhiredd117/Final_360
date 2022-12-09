/*********** util.c file ****************/
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <ext2fs/ext2_fs.h>
#include <string.h>
#include <libgen.h>
#include <sys/stat.h>
#include <time.h>

#include "type.h"

/**** globals defined in main.c file ****/
extern MINODE minode[NMINODE];
extern MINODE *root;
extern PROC   proc[NPROC], *running;

extern char gpath[128];
extern char *name[64];
extern int n;

extern int fd, dev;
extern int nblocks, ninodes, bmap, imap, iblk;

extern char line[128], cmd[32], pathname[128];

int get_block(int dev, int blk, char *buf)
{
   lseek(dev, (long)blk*BLKSIZE, 0);
   read(dev, buf, BLKSIZE);
}   

int put_block(int dev, int blk, char *buf)
{
   lseek(dev, (long)blk*BLKSIZE, 0);
   write(dev, buf, BLKSIZE);
}   

int tokenize(char *pathname)
{
  int i;
  char *s;
  printf("tokenize %s\n", pathname);

  strcpy(gpath, pathname);   // tokens are in global gpath[ ]
  n = 0;

  s = strtok(gpath, "/");
  while(s){
    name[n] = s;
    n++;
    s = strtok(0, "/");
  }
  name[n] = 0;
  
  for (i= 0; i<n; i++)
    printf("%s  ", name[i]);
  printf("\n");
}

// Searches in memory minodes and updates the refCount  (number of processes that are using the minode. 
//The returned minode is unique, i.e. only one copy of the INODE exists in memory. 
//In a real file system, the returned minode is locked for exclusive use until it is either released or unlocked. 
MINODE *iget(int dev, int ino)
{
  int i;
  MINODE *mip;
  char buf[BLKSIZE];
  int blk, offset;
  INODE *ip;

  for (i=0; i<NMINODE; i++){
    mip = &minode[i];
    if (mip->refCount && mip->dev == dev && mip->ino == ino){
       mip->refCount++;
       //printf("found [%d %d] as minode[%d] in core\n", dev, ino, i);
       return mip;
    }
  }
    
  for (i=0; i<NMINODE; i++){
    mip = &minode[i];
    if (mip->refCount == 0){
       //printf("allocating NEW minode[%d] for [%d %d]\n", i, dev, ino);
       mip->refCount = 1;
       mip->dev = dev;
       mip->ino = ino;

       // get INODE of ino into buf[ ]    
       blk    = (ino-1)/8 + iblk;
       offset = (ino-1) % 8;

       //printf("iget: ino=%d blk=%d offset=%d\n", ino, blk, offset);

       get_block(dev, blk, buf);    // buf[ ] contains this INODE
       ip = (INODE *)buf + offset;  // this INODE in buf[ ] 
       // copy INODE to mp->INODE
       mip->INODE = *ip;
       return mip;
    }
  }   
  printf("PANIC: no more free minodes\n");
  return 0;
}
//releases a used minode pointed by mip. 
//Each minode has a refCount, which represents the number of users that are using the minode. 
//iput() decrements the refCount by 1. If the refCount is non-zero, meaning that the minode still has other users, the caller simply returns. 
//If the caller is the last user of the minode (refCount ¼ 0), the INODE is written back to disk if it is modified (dirty).


void iput(MINODE *mip)  // iput(): release a minode
{
 int i, block, offset, iblock;
 char buf[BLKSIZE];
 INODE *ip;
 

 if (mip==0) 
     return;

 mip->refCount--;
 
 if (mip->refCount > 0) return;
 if (!mip->dirty)       return;
 
 /* write INODE back to disk */
 /**************** NOTE ******************************
  For mountroot, we never MODIFY any loaded INODE
                 so no need to write it back
  FOR LATER WROK: MUST write INODE back to disk if refCount==0 && DIRTY

  Write YOUR code here to write INODE back to disk
 *****************************************************/

   // write INODE back to disk
   block = ((mip->ino - 1) / 8) + iblk;
   offset = (mip->ino - 1) % 8;

   

   // get block containing this inode
   get_block(mip->dev, block, buf);
   //Point to the INODE
   ip = (INODE *)buf + offset; 
   *ip = mip->INODE;

    //Write to disk finally
   put_block(mip->dev, block, buf);
   //midalloc(mip);

} 
//search for the token strings in successive directories
int search(MINODE *mip, char *name)
{
   int i; 
   char *cp, c, sbuf[BLKSIZE], temp[256];
   DIR *dp;
   INODE *ip;

   printf("search for %s in MINODE = [%d, %d]\n", name,mip->dev,mip->ino);
   ip = &(mip->INODE);

   /*** search for name in mip's data blocks: ASSUME i_block[0] ONLY ***/

   get_block(dev, ip->i_block[0], sbuf);
   dp = (DIR *)sbuf;
   cp = sbuf;
   printf("  ino   rlen  nlen  name\n");

   while (cp < sbuf + BLKSIZE){
     strncpy(temp, dp->name, dp->name_len); // dp->name is NOT a string
     temp[dp->name_len] = 0;                // temp is a STRING
     printf("%4d  %4d  %4d    %s\n", 
	    dp->inode, dp->rec_len, dp->name_len, temp); // print temp !!!

     if (strcmp(temp, name)==0){            // compare name with temp !!!
        printf("found %s : ino = %d\n", temp, dp->inode);
        return dp->inode;
     }

     cp += dp->rec_len;
     dp = (DIR *)cp;
   }
   return 0;
}
//Implements file system tree traversal algorithm. 
//Assuming the file system resides on a single root device so no mounted devices and mounting point crossings. 
//It will return the (dev, ino) of a pathname

int getino(char *pathname) // return ino (number) of pathname
{
  int i, ino, blk, offset;
  char buf[BLKSIZE];
  INODE *ip;
  MINODE *mip;

  printf("getino: pathname=%s\n", pathname);
  if (strcmp(pathname, "/")==0)
      return 2;
  
  // starting mip = root OR CWD
  if (pathname[0]=='/')
     mip = root;
  else
     mip = running->cwd;

  mip->refCount++;         // because we iput(mip) later
  
  tokenize(pathname);

  for (i=0; i<n; i++){
      printf("===========================================\n");
      printf("getino: i=%d name[%d]=%s\n", i, i, name[i]);
 
      ino = search(mip, name[i]);

      if (ino==0){
         iput(mip);
         printf("name %s does not exist\n", name[i]);
         return 0;
      }

      iput(mip);
      mip = iget(dev, ino);
   }

   iput(mip);
   return ino;
}

// These 2 functions are needed for pwd()
int findmyname(MINODE *parent, u32 myino, char myname[ ]) 
{
  // WRITE YOUR code here
  // search parent's data block for myino; SAME as search() but by myino
  // copy its name STRING to myname[ ]
  char *cp, c, sbuf[BLKSIZE], temp[256];
    DIR *dp;
    MINODE *ip = parent;

    //Loop through at max 12 blocks of the parent node searching
    for (int i = 0; i < 12; ++i) {

        //Make sure the block is around
        if (ip->INODE.i_block[i] != 0) {

            //Copy the blcok into the buffer defined above
            get_block(ip->dev, ip->INODE.i_block[i], sbuf);

            //Set up the cariables to use while searching through the block
            dp = (DIR *) (sbuf); //dir pointer
            cp = sbuf;

            //Loop through the block
            while (cp < sbuf + BLKSIZE) {
                //Check if the ino matches
                if (dp->inode == myino) {
                    //Copy the name to the pointer we were given
                    strncpy(myname, dp->name, dp->name_len);
                    myname[dp->name_len] = 0; //get rid of \n

                    //Return successfully and stop looping
                    return 0;
                }

                //Advance through the block
                cp += dp->rec_len;
                dp = (DIR *) (cp);
            }
        }
    }

    return -1;
}



int findino(MINODE *mip, u32 *myino) // myino = i# of . return i# of ..
{
  // mip points at a DIR minode
  // WRITE your code here: myino = ino of .  return ino of ..
  // all in i_block[0] of this DIR INODE.

  //Create buffer and temporary pointer
  char buffer[BLKSIZE], *cp;
  //Create variable to store directory
  DIR *dp;
  
  //Read the first block in the MINODE
  get_block(mip->dev, mip->INODE.i_block[0], buffer);
  
  //Set up dp & cp
   dp = (DIR *) buffer;
   cp = buffer;

   //Write to the inode pointer supplied
   *myino = dp->inode;

   //Advance the temporary pointer in the buffer
   cp += dp->rec_len;

   //Cast dp to the new directory found
   dp = (DIR *) cp;

   //Return the inode found
   return dp->inode;
}



