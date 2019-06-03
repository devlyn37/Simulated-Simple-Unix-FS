#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

const short BLOCK_SIZE;
const int NUM_BLOCKS;
const int INODE_SIZE;
const int NUM_INODE;
const int INODE_REF_SIZE;
const int ENTRY_SIZE;

void readBlock(FILE *disk, int blockNum, char *buffer);

void writeBlock(FILE *disk, int blockNum, char *data, int size);