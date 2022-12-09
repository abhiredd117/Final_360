#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <ext2fs/ext2_fs.h>
#include <string.h>
#include <libgen.h>
#include <sys/stat.h>
#include <time.h>

#include "type.h"

extern MINODE *iget();

extern SUPER *sp;
extern GD    *gp;
extern INODE *ip;
extern DIR   *dp;

extern MINODE minode[NMINODE];
extern MINODE *root;
extern PROC   proc[NPROC], *running;

extern char gpath[128]; // global for tokenized components
extern char *name[64];  // assume at most 64 components in pathname
extern int   n;         // number of component strings

extern int  fd, dev;
extern int  nblocks, ninodes, bmap, imap, iblk;
extern char line[128], cmd[32], pathname[128];

//Tests if the block is free or not
int tst_bit(char *buf, int bit){
    return buf[bit/8] & (1 << (bit % 8));
}

//Makes the indicated block free
int clr_bit(char *buf, int bit){ // clear bit in char buf[BLKSIZE]
    buf[bit/8] &= ~(1 << (bit%8));
}

//Makes the indicated block occupied
int set_bit(char *buf, int bit){
    buf[bit/8] |= (1 << (bit % 8));
}

int decFreeInodes(int dev)
{
    char buf[BLKSIZE];

    // dec free inodes count in SUPER and GD
    get_block(dev, 1, buf);
    sp = (SUPER *)buf;
    sp->s_free_inodes_count--;
    put_block(dev, 1, buf);
    get_block(dev, 2, buf);
    gp = (GD *)buf;
    gp->bg_free_inodes_count--;
    put_block(dev, 2, buf);
}

int decFreeBlocks(int dev)
{
    char buf[BLKSIZE];

    get_block(dev, 1, buf);
    sp = (SUPER *)buf;
    sp->s_free_blocks_count--;
    put_block(dev, 1, buf);

    get_block(dev, 2, buf);
    gp = (GD *)buf;
    gp->bg_free_blocks_count--;
    put_block(dev, 2, buf);
}

int ialloc(int dev)  // allocate an inode number from inode_bitmap
{
    int  i;
    char buf[BLKSIZE];

    // read inode_bitmap block
    get_block(dev, imap, buf);

    for (i=0; i < ninodes; i++){ // use ninodes from SUPER block
        if (tst_bit(buf, i)==0){
            set_bit(buf, i);
            put_block(dev, imap, buf);

            decFreeInodes(dev);

            printf("allocated ino = %d\n", i+1); // bits count from 0; ino from 1
            return i+1;
        }
    }
    return 0;
}

 //  balloc allocates a FREE disk block number
 // 
 int balloc(int dev)
 { //returns a FREE disk block number  NOTE: Not 100% sure if this works
     int i;
     char buf[BLKSIZE];

     get_block(dev, bmap, buf);

     for (i = 0; i < nblocks; i++)
     {
         if (tst_bit(buf, i) == 0)
         {
             set_bit(buf, i);
             decFreeBlocks(dev);
             put_block(dev, bmap, buf);
             printf("Free disk block at %d\n", i + 1); // bits count from 0; ino from 1
             return i + 1;
         }
     }
     return 0;
 }
 
int incFreeInodes(int dev)
{
    char buf[BLKSIZE];

    // inc free inodes count in SUPER and GD
    get_block(dev, 1, buf);
    sp = (SUPER *)buf;
    sp->s_free_inodes_count++;
    put_block(dev, 1, buf);

    get_block(dev, 2, buf);
    gp = (GD *)buf;
    gp->bg_free_inodes_count++;
    put_block(dev, 2, buf);
}

//Blocks house inodes
int incFreeBlocks(int dev)
{
    char buf[BLKSIZE];

    get_block(dev, 1, buf);
    sp = (SUPER *)buf;
    sp->s_free_blocks_count++;
    put_block(dev, 1, buf);

    get_block(dev, 2, buf);
    gp = (GD *)buf;
    gp->bg_free_blocks_count++;
    put_block(dev, 2, buf);
}


int idalloc(int dev, int ino)
{
    int i;
    char buf[BLKSIZE];

    if (ino > ninodes){
        printf("inumber %d out of range\n", ino);
        return -1;
    }

    // get inode bitmap block
    get_block(dev, imap, buf);
    clr_bit(buf, ino-1);

    // write buf back
    put_block(dev, imap, buf);

    // update free inode count in SUPER and GD
    incFreeInodes(dev);
}

int bdalloc(int dev, int bno)
{
    char buf[BLKSIZE]; // a sweet buffer

    get_block(dev, bmap, buf); // get the block
    clr_bit(buf, bno - 1);     // clear the bits to 0
    put_block(dev, bmap, buf); // write the block back
    incFreeBlocks(dev);        // increment the free block count
    return 0;
}
