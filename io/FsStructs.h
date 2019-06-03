#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

struct inode
{
    int sizeOfFile;
    int flags;
    short location;
    short *dataAddresses;
    short singleIndirectBlock;
};

struct inodeRef
{
    short id;
    short available;
    short address;
};

struct indirectionBlock
{
    short *dataAddresses;
};

struct directoryEntry
{
    short available;
    short address;
    char *name;
};

struct directory
{
    struct directoryEntry **entries;
};