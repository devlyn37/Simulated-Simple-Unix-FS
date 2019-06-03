#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
const short BLOCK_SIZE = 512;
const int NUM_BLOCKS = 4096;
const int INODE_SIZE = 32;
const int NUM_INODE = 128; //(NUM_BLOCKS * 512) / 16384;
const int INODE_REF_SIZE = 6;
const int ENTRY_SIZE = 32;

void readBlock(FILE *disk, int blockNum, char *buffer)
{
    if (blockNum == -1)
    {
        printf("What your trying to read is empty/hasn't been initialized\n");
    }

    fseek(disk, blockNum * BLOCK_SIZE, SEEK_SET);
    fread(buffer, BLOCK_SIZE, 1, disk);
}

void writeBlock(FILE *disk, int blockNum, char *data, int size)
{
    fseek(disk, blockNum * BLOCK_SIZE, SEEK_SET);
    fwrite(data, size, 1, disk); // Note: this will overwrite existing data in the block
}
