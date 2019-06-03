#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "../io/FsStructs.h"
#include "../io/system.h"
#include "../io/disk.h"

int main(int argc, char *argv[])
{
    printf("This is a test for reading and writing blocks from a simulated disk\n");
    char *test = "If You can read me it worked!";
    char *writeData = calloc(1, BLOCK_SIZE);
    memcpy(writeData, test, strlen(test));

    printf("writing block to disk!\n");
    FILE *disk = fopen(SimulatedDisk, "wb+");
    writeBlock(disk, 1, writeData, BLOCK_SIZE);

    printf("reading block from disk!\n");
    char *readData = calloc(1, BLOCK_SIZE);
    readBlock(disk, 1, readData);

    printf("Heres the data read from block: %s\n", readData);
    fclose(disk);
}