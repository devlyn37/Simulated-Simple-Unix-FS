#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "../io/FsStructs.h"
#include "../io/system.h"
#include "../io/disk.h"

void testAddDirectory()
{
    printf("\n\n\ntesting add directory functionality\n\n");
    initializeSimulatedDisk();
    fileSystemOverview();

    printf("Adding a new directory dir1 to the root directory\n");
    mkDir("/", "dir1");
    fileSystemOverview();

    printf("Adding a new directory dir2 to dir1\n");
    mkDir("/dir1", "dir2");
    fileSystemOverview();

    printf("Adding new directory dir3 to dir2\n");
    mkDir("dir1/dir2", "dir3");
    fileSystemOverview();

    printf("Adding new directory dir4 to dir2\n");
    mkDir("/dir1/dir2", "dir4");
    fileSystemOverview();
}

void testAddAndReadFile()
{
    printf("\n\n\ntesting add file and read file functionality\n\n");
    initializeSimulatedDisk();
    fileSystemOverview();
    printf("making a couple directories to test this within\n");
    mkDir("/", "dir1");
    mkDir("/dir1", "dir2");

    printf("Making the file within dir2, path: dir1/dir2\n");
    mkFile("disk/test", "/dir1/dir2", "testFile");

    printf("Reading file in path dir1/dir2 named testFile\n");
    printf("If you see the lyrics to the song All Star by Smash Mouth 6 times then everything is working\n");
    printf("File Contents:\n");
    char *test = readFile("dir1/dir2", "testFile");

    printf("%s\n", test);
    printf("Done reading file contents\n");
    fileSystemOverview();
}

void testDeleteFile()
{
    printf("\n\n\ntesting delete file functionality\n\n");
    initializeSimulatedDisk();
    fileSystemOverview();
    printf("making a couple directories to test this within\n");
    mkDir("/", "dir1");
    mkDir("/dir1", "dir2");

    printf("Making the file within dir2, path: dir1/dir2\n");
    mkFile("disk/test", "/dir1/dir2", "testFile");
    fileSystemOverview();

    printf("Deleting File we just added\n");
    deleteFile("/dir1/dir2", "testFile");
    fileSystemOverview();
}

void testDeleteDirectory()
{
    printf("\n\n\nTesting delete directory functionality\n\n");
    initializeSimulatedDisk();
    fileSystemOverview();
    printf("making a couple directories to test this within\n");
    mkDir("/", "dir1");
    mkDir("/dir1", "dir2");
    mkDir("/dir1/dir2", "dir3");
    fileSystemOverview();

    printf("making a file in each directory\n");
    mkFile("disk/test", "/dir1/dir2/dir3", "testFile");
    mkFile("disk/test", "/dir1/dir2", "testFile");
    mkFile("disk/test", "/dir1", "testFile");
    fileSystemOverview();

    printf("deleting dir1 which contains other directories and files which will be recursivly deleted\n");
    deleteDirectory("/", "dir1");
    fileSystemOverview();
}

int main(int argc, char *argv[])
{
    printf("This is a test for the file systems main functions\n");
    initializeSimulatedDisk();
    testAddDirectory();
    testAddAndReadFile();
    testDeleteFile();
    testDeleteDirectory();
}