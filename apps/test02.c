#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "../io/FsStructs.h"
#include "../io/system.h"
#include "../io/disk.h"

int main(int argc, char *argv[])
{
    printf("This is a test for initializing the simulated disk!\n");
    initializeSimulatedDisk();
    printf("After the disk is initialized there should be 127 inactive inodes and 1 thats points to a directory\n");
    fileSystemOverview();
}