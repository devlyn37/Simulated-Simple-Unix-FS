#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "FsStructs.h"
#include "disk.h"
#include "bit-ops.h" // source: http://www.mathcs.emory.edu/~cheung/Courses/255/Syllabus/1-C-intro/bit-array.html

//inode flags
const int DIRECTORY = 2;
const int REGFILE = 1;
const int AVAILABLE = 0;

//blocks
const int SUPERBLOCK = 0;
const int FREEBLOCKLIST = 1;
const int INODESTART = 2;
const int INODEEND = 10;
const int ROOTDIR = 11;
const int FREESTART = 12;
const int FREEEND = 4096;

int debug = 0;

//apparently longs are 8 bytes on macs and linux machines
//ints are 4 on all operating systems on intel chips though

struct directoryEntry *createDirectoryEntry(short available, short address, char *name)
{
    struct directoryEntry *entry = malloc(sizeof(struct directoryEntry));
    entry->available = available;
    entry->address = address;
    entry->name = name;
    return entry;
}

struct directory *createDirectory(struct directoryEntry **entries)
{
    struct directory *dir = (struct directory *)malloc(sizeof(struct directory));
    dir->entries = entries;
    return dir;
}

struct inode *createInode(int sizeOfFile, int flags, short location, short *dataAddresses, short singleIndirectBlock)
{
    struct inode *newInode = (struct inode *)malloc(sizeof(struct inode));
    newInode->dataAddresses = malloc(sizeof(short) * 10);
    memcpy(newInode->dataAddresses, dataAddresses, sizeof(short) * 10);
    newInode->sizeOfFile = sizeOfFile;
    newInode->flags = flags;
    newInode->location = location;
    newInode->singleIndirectBlock = singleIndirectBlock;
    return newInode;
}

struct indirectionBlock *createIndirectionBlock(short *addresses)
{
    struct indirectionBlock *idb = malloc(sizeof(struct indirectionBlock));
    idb->dataAddresses = malloc(sizeof(short) * 256);
    memcpy(idb->dataAddresses, addresses, sizeof(short) * 256);
    return idb;
}

struct indirectionBlock *createEmptyIndirectionBlock()
{
    short dataAddresses[256];
    for (int i = 0; i < 256; i++)
    {
        dataAddresses[i] = -1;
    }
    return createIndirectionBlock(dataAddresses);
}

//Creates an empty Inode, Inodes have a flag value of 0 until there assigned to either a file
//or a directory and they'll be labeled as such
struct inode *createEmptyInode()
{
    short dataAddresses[10] = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1};
    return createInode(0, 0, AVAILABLE, dataAddresses, -1);
}

//the convention for entries is to have them contain an empty string if there not occupied
struct directoryEntry *createEmptyDirectoryEntry()
{
    struct directoryEntry *entry = malloc(sizeof(struct directoryEntry));
    entry->address = (short)-1;
    entry->available = (short)1;

    char *name = malloc(sizeof(char) * 28);
    memcpy(name, "", 28);

    entry->name = name;
    return entry;
}

//Just a bunch of empty entries
struct directory *createEmptyDirectory()
{
    struct directory *dir = malloc(sizeof(struct directory));
    struct directoryEntry **entries = malloc(sizeof(struct directoryEntry) * 16);
    for (int i = 0; i < 16; i++)
    {
        entries[i] = createEmptyDirectoryEntry();
    }
    dir->entries = entries;
    return dir;
}

/*The following set of functions take
structs and turn them into data that can be written into disk
and vice versa*/

char *indirectionBlockToBytes(struct indirectionBlock *idb)
{
    char *data = malloc(BLOCK_SIZE);
    memcpy(data, idb->dataAddresses, sizeof(short) * 256);
    return data;
}

struct indirectionBlock *bytesToIndirectionBlock(char *data)
{
    struct indirectionBlock *idb = createEmptyIndirectionBlock();
    memcpy(idb->dataAddresses, data, sizeof(short) * 256);
    return idb;
}

char *directoryEntryToBytes(struct directoryEntry *entry)
{
    char *data = malloc(ENTRY_SIZE);

    int offset = 0;
    memcpy(data, &entry->available, sizeof(short));
    offset += sizeof(short);
    memcpy(data + offset, &entry->address, sizeof(short));
    offset += sizeof(short);
    memcpy(data + offset, entry->name, ENTRY_SIZE - offset);

    return data;
}

struct directoryEntry *BytesToDirectoryEntry(char *data)
{
    struct directoryEntry *entry = createEmptyDirectoryEntry();

    int offset = 0;
    memcpy(&entry->available, data, sizeof(short));
    offset += sizeof(short);
    memcpy(&entry->address, data + offset, sizeof(short));
    offset += sizeof(short);
    memcpy(entry->name, data + offset, ENTRY_SIZE - offset);

    return entry;
}

char *directoryToBytes(struct directory *dir)
{
    char *data = malloc(BLOCK_SIZE);
    int offset = 0;
    for (int i = 0; i < 16; i++)
    {
        memcpy(data + offset, directoryEntryToBytes(dir->entries[i]), ENTRY_SIZE);
        offset += ENTRY_SIZE;
    }
    return data;
}

struct directory *bytesToDirectory(char *data)
{
    struct directory *dir = createEmptyDirectory();

    char *entry = malloc(ENTRY_SIZE);
    int offset = 0;
    for (int i = 0; i < 16; i++)
    {
        memcpy(entry, data + offset, ENTRY_SIZE);
        dir->entries[i] = BytesToDirectoryEntry(entry);
        offset += ENTRY_SIZE;
    }

    return dir;
}

//take inode struct and turn it into data to go into disk
//tested and works
char *inodeToBytes(struct inode *inode)
{
    //printf("formatting inode for disk!!!\n");
    //printf("first, allocating %d bytes for it\n", INODE_SIZE);
    char *data = malloc(INODE_SIZE);
    int offset = 0;
    //printf("copying over int sizeOfFile from struct\n");
    memcpy(data, &inode->sizeOfFile, sizeof(int));
    offset += sizeof(int);
    //printf("copying over int flags, offset in allocated mem = %d\n", offset);
    memcpy(data + offset, &inode->flags, sizeof(int));
    offset += sizeof(int);
    //printf("copying over doubleindirectblock, offset in allocated mem = %d\n", offset);
    memcpy(data + offset, &inode->location, sizeof(short));
    offset += sizeof(short);
    //printf("copying over data addresses, offset in allocated mem = %d\n", offset);
    memcpy(data + offset, inode->dataAddresses, sizeof(short) * 10);
    offset += (sizeof(short) * 10);
    //printf("copying over singleindirectblock, offset in allocated mem = %d\n", offset);
    memcpy(data + offset, &inode->singleIndirectBlock, sizeof(short));
    offset += sizeof(short);
    return data;
}

//converts data pulled from file to inode struct
struct inode *bytesToInode(char *data)
{
    // struct inode *inode = (struct inode *)malloc(sizeof(struct inode));
    // inode->dataAddresses = malloc(sizeof(short *) * 10);
    struct inode *inode = createEmptyInode();

    int offset = 0;
    memcpy(&inode->sizeOfFile, data, sizeof(int));
    offset += sizeof(int);
    memcpy(&inode->flags, data + offset, sizeof(int));
    offset += sizeof(int);
    memcpy(&inode->location, data + offset, sizeof(short));
    offset += sizeof(short);
    memcpy(inode->dataAddresses, data + offset, sizeof(short) * 10);
    offset += sizeof(short) * 10;
    memcpy(&inode->singleIndirectBlock, data + offset, sizeof(short));
    offset += sizeof(short);
    return inode;
}

/*Pretty standard print functions for debugging*/
void printInode(struct inode *inode)
{
    printf("inode: size of file: %d\nflags:%d\ninode location: %d\nsingle indirection block address: %d\n", inode->sizeOfFile, inode->flags, inode->location, inode->singleIndirectBlock);
    for (int i = 0; i < 10; i++)
    {
        printf("address: %d\n", inode->dataAddresses[i]);
    }
}

void printDirectory(struct directory *dir)
{
    for (int i = 0; i < 16; i++)
    {
        printf("entry: %d, %d, %s\n", dir->entries[i]->available, dir->entries[i]->address, dir->entries[i]->name);
    }
}

void printIndirectionBlock(struct indirectionBlock *idb)
{
    printf("indirectionBlock\n");
    for (int i = 0; i < BLOCK_SIZE / sizeof(short); i++)
    {
        printf("address: %d\n", idb->dataAddresses[i]);
    }
}

//get the block number from your address
//floor of address/blocksize
int getBlockNum(int address)
{
    return address / BLOCK_SIZE;
}

//get the offset from the start of a block given an inode
int getOffset(int address)
{
    int blockNum = getBlockNum(address);
    return address - (blockNum * BLOCK_SIZE);
}

//write an updated inode into disk
void updateInode(FILE *disk, struct inode *inode)
{
    //printf("updating inode with address %d\n\n\n\n\n", address);
    //First find and read block the inode is in
    int blockNum = getBlockNum(inode->location);
    char *blockWithInode = malloc(BLOCK_SIZE);
    readBlock(disk, blockNum, blockWithInode);

    //Second, modify the block with updated inode info
    int offset = getOffset(inode->location);
    char *inodeData = inodeToBytes(inode);
    memcpy(blockWithInode + offset, inodeData, INODE_SIZE);

    //Third, write the updated block back to the disk
    writeBlock(disk, blockNum, blockWithInode, BLOCK_SIZE);

    //Finally, free stuff and your done
    free(inodeData);
    free(blockWithInode);
}

//write an updated directory into disk given its inode
void updateDir(FILE *disk, struct inode *inode, struct directory *dir)
{
    if (inode->flags != DIRECTORY)
    {
        printf("This is the incorrect inode or its flags are not set to indicate it references a directory\n");
        return;
    }

    short blockOfDir = inode->dataAddresses[0];
    if (blockOfDir == -1)
    {
        printf("inode doesn't have address set yet, the convention is to keep the pointer to the directory block in the first data slot\n");
        return;
    }

    //writing to disk is easy because directories are always a block in size
    writeBlock(disk, blockOfDir, directoryToBytes(dir), BLOCK_SIZE);
}

//read a directory from disk given its inode
struct directory *getDirectory(FILE *disk, struct inode *inode)
{
    if (inode->flags != DIRECTORY)
    {
        printf("This is the incorrect inode or its flags are not set to indicate it references a directory\n");
        return NULL;
    }

    short blockOfDir = inode->dataAddresses[0];
    if (blockOfDir == -1)
    {
        printf("inode doesn't have first block number set yet, the convention is to keep the pointer to the directory block in the first data slot\n");
        return NULL;
    }

    char *dirData = malloc(BLOCK_SIZE);
    readBlock(disk, blockOfDir, dirData);
    struct directory *dir = bytesToDirectory(dirData);
    free(dirData);
    return dir;
}
int *readFreeBlockList(FILE *disk)
{
    //needs to be an int or it gets read differently
    char *block = malloc(BLOCK_SIZE);
    readBlock(disk, 1, block);
    int *freeBlockList = (int *)block;
    return freeBlockList;
}

int findFreeBlock(FILE *disk)
{
    int blockNum = 0;
    int *freeBlockList = readFreeBlockList(disk);

    for (blockNum = 0; blockNum < 4096; blockNum++)
    {
        if (TestBit(freeBlockList, blockNum))
        {
            free(freeBlockList);
            return blockNum;
        }
    }
    free(freeBlockList);
    return -1;
}

//Can return null.
//find an entry in a directory according to the name in the parameters
struct directoryEntry *findDirectoryEntry(struct directory *dir, char *name)
{
    // printf("Finding a directory entry named %s in this directory:\n", name);
    // printDirectory(dir);
    //grab an available entry in the directory;
    struct directoryEntry *foundEntry = NULL;
    for (int i = 0; i < 16 && foundEntry == NULL; i++)
    {
        if (strcmp(name, dir->entries[i]->name) == 0)
        {
            foundEntry = dir->entries[i];
        }
    }

    if (foundEntry == NULL)
    {
        printf("Couldn't find a directory that fits your criteria\n");
    }

    return foundEntry;
}

//Can return null.
//look for an empty entry in a directory, empty entries always contain an empty string by convention
struct directoryEntry *findEmptyDirectoryEntry(struct directory *dir)
{
    return findDirectoryEntry(dir, "");
}

//update a block to be available on the free block list
void markBlockAvailable(FILE *disk, int blockNum)
{
    int *freeBlockList = readFreeBlockList(disk);
    SetBit(freeBlockList, blockNum);
    writeBlock(disk, FREEBLOCKLIST, (char *)freeBlockList, BLOCK_SIZE);
    free(freeBlockList);
}

//mark a block to be unavailable on the free block list
void markBlockUnavailable(FILE *disk, int blockNum)
{
    int *freeBlockList = readFreeBlockList(disk);
    ClearBit(freeBlockList, blockNum);
    writeBlock(disk, FREEBLOCKLIST, (char *)freeBlockList, BLOCK_SIZE);
    free(freeBlockList);
}

//Look through the inode blocks to find an available inode, if there's an available inode
//return a pointer to inode struct, otherwise return NULL;
struct inode *findFreeInode(FILE *disk)
{
    struct inode *currInode;
    char *currInodeBlock = malloc(BLOCK_SIZE);
    int blockOffset;

    for (int blockNumber = INODESTART; blockNumber <= INODEEND; blockNumber++)
    {
        readBlock(disk, blockNumber, currInodeBlock);
        for (blockOffset = 0; blockOffset < BLOCK_SIZE; blockOffset += INODE_SIZE)
        {
            currInode = bytesToInode(currInodeBlock + blockOffset);
            if (currInode->flags == AVAILABLE)
            {
                printf("Found a free Inode, blocknumber: %d, inode: %d\n", blockNumber, blockOffset / INODE_SIZE);
                free(currInodeBlock);
                return currInode;
            }
            //memory is allocated for every time a node is read from vdisk, so it needs to be freed
            free(currInode);
        }
        free(currInodeBlock);
    }
    //if this point is reached theres no free inodes;
    printf("No more free Inodes left!\n");
    return NULL;
}

//the root inode is the first inode
struct inode *getRootInode(FILE *disk)
{
    char *inodeBlock = malloc(BLOCK_SIZE);
    char *rootInodeData = malloc(INODE_SIZE);
    readBlock(disk, INODESTART, inodeBlock);
    memcpy(rootInodeData, inodeBlock, INODE_SIZE);

    struct inode *rootInode = bytesToInode(rootInodeData);
    free(inodeBlock);
    free(rootInodeData);
    return rootInode;
}

struct directory *getRootDir(FILE *disk)
{
    char *dirData = malloc(BLOCK_SIZE);
    readBlock(disk, ROOTDIR, dirData);
    struct directory *dir = bytesToDirectory(dirData);

    free(dirData);
    return dir;
}

//given its address, read an inode from disk
struct inode *getInode(FILE *disk, short address)
{
    char *inodeBlock = malloc(BLOCK_SIZE);
    char *inodeData = malloc(INODE_SIZE);
    int blockNum = getBlockNum(address);
    readBlock(disk, blockNum, inodeBlock);

    int offset = getOffset(address);
    memcpy(inodeData, inodeBlock + offset, INODE_SIZE);

    struct inode *inode = bytesToInode(inodeData);
    free(inodeBlock);
    free(inodeData);

    return inode;
}

//points the first entry in an inode to a freedata block
//Then writes the directory to disk in that block
void addDirToInode(FILE *disk, struct inode *inode, struct directory *dir)
{
    short freeBlock = findFreeBlock(disk);
    inode->dataAddresses[0] = freeBlock;
    markBlockUnavailable(disk, freeBlock);
    //printf("writing data from directory to block %hd which the inode points to\n", freeBlock);
    writeBlock(disk, freeBlock, directoryToBytes(dir), BLOCK_SIZE);
}

//writes data provided to disk and points appropriate inode
//addresses to that data, If the data is large enough to justify it,
//this function will create an indirection block for the inode.
//When it comes to robustness this is gonna be where things
//get complicated
void writeToInode(FILE *disk, struct inode *inode, char *data)
{
    //for now assume things are just gonna be text
    int bytesLeft = strlen(data);
    if (debug)
        printf("length of data in bytes: %d\n", bytesLeft);

    int freeBlock;
    int offset = 0;
    int bytesToWrite;
    for (int i = 0; i < 10 && bytesLeft > 0; i++)
    {
        bytesToWrite = BLOCK_SIZE;
        if (bytesLeft < BLOCK_SIZE)
            bytesToWrite = bytesLeft;

        freeBlock = findFreeBlock(disk);
        if (debug)
            printf("allocating block %d to store data\n", freeBlock);
        inode->dataAddresses[i] = (short)freeBlock;
        markBlockUnavailable(disk, freeBlock);
        writeBlock(disk, freeBlock, data + offset, bytesToWrite);
        offset += bytesToWrite;
        bytesLeft -= bytesToWrite;
        if (debug)
            printf("successfully filled block %d, %d bytes left of data to write\n", freeBlock, bytesLeft);
    }

    if (debug)
        printf("done writing to inode datablocks, bytes left write = %d\n", bytesLeft);
    if (bytesLeft == 0)
    {
        if (debug)
            printf("finished writing everything into disk\n");
        return;
    }

    if (debug)
        printf("There's still data to write but were out of spaces in the inode, creating an indirection block\n");
    struct indirectionBlock *idb = createEmptyIndirectionBlock();
    int indirectionBlockNum = findFreeBlock(disk);
    inode->singleIndirectBlock = indirectionBlockNum;
    markBlockUnavailable(disk, indirectionBlockNum);
    if (debug)
        printf("allocating block %d for the indirection block\n", indirectionBlockNum);

    for (int i = 0; i < 256 && bytesLeft > 0; i++)
    {
        bytesToWrite = BLOCK_SIZE;
        if (bytesLeft < BLOCK_SIZE)
            bytesToWrite = bytesLeft;

        freeBlock = findFreeBlock(disk);
        if (debug)
            printf("allocating block %d to store data\n", freeBlock);
        idb->dataAddresses[i] = (short)freeBlock;
        markBlockUnavailable(disk, freeBlock);
        writeBlock(disk, freeBlock, data + offset, bytesToWrite);
        offset += bytesToWrite;
        bytesLeft -= bytesToWrite;
        if (debug)
            printf("successfully filled block %d, %d bytes left of data to write\n", freeBlock, bytesLeft);
    }

    if (debug)
        printIndirectionBlock(idb);

    if (bytesLeft != 0)
    {
        if (debug)
            printf("Thats as large as a file can get, finishing with %d bytes left from file\n", bytesLeft);
        return;
    }

    if (debug)
        printf("finished writing file to disk, now writing indirection block to disk\n");
    char *idbData = indirectionBlockToBytes(idb);
    writeBlock(disk, indirectionBlockNum, idbData, BLOCK_SIZE);
    if (debug)
        printf("Done everything! have a good rest of your day!\n");
}

char *readInode(FILE *disk, struct inode *inode)
{
    if (debug)
        printf("reading inode contents\n");
    if (inode->flags == DIRECTORY)
    {
        if (debug)
            printf("this inode is for a directory returning string to reflect that\n");
        return "directory";
    }

    if (inode->sizeOfFile <= 0)
    {
        if (debug)
            printf("There's no content to read, returning string to reflect that\n");
        return "no content, filesize <= 0";
    }

    char *data = malloc(inode->sizeOfFile + 1);

    short dataBlockNum;
    int bytesLeft = inode->sizeOfFile;
    int offset = 0;
    for (int i = 0; i < 10; i++)
    {
        dataBlockNum = inode->dataAddresses[i];
        if (dataBlockNum == -1)
        {
            if (bytesLeft <= 0)
            {
                if (debug)
                    printf("finished reading in content from disk at data address %d\n", i);
                break;
            }
            else
            {
                printf("Reached a null block pointer (-1) but according to the file size\n");
                printf("of the inode there should be %d bytes of data to read, therefore something\n", bytesLeft);
                printf("must be corrupted, returning an NULL\n");
                return NULL;
            }
        }

        readBlock(disk, dataBlockNum, data + offset);
        offset += BLOCK_SIZE;
        bytesLeft -= BLOCK_SIZE;
    }

    if (debug)
        printf("done reading from data block pointers in inode\n");
    if (bytesLeft <= 0)
    {
        if (debug)
            printf("File is fully read in returning it\n");
        return data;
    }

    if (debug)
        printf("There's more data to read so I'm grabbing the inodes indirection block\n");
    if (inode->singleIndirectBlock == -1)
    {
        printf("there's %d more bytes left to read but there's no indirection block\n", bytesLeft);
        printf("assigned to this inode, something must be corrected, returning NULL\n");
        return NULL;
    }

    if (debug)
        printf("%hd\n\n\n", inode->singleIndirectBlock);
    char *idbBlock = malloc(BLOCK_SIZE);
    readBlock(disk, inode->singleIndirectBlock, idbBlock);
    struct indirectionBlock *idb = bytesToIndirectionBlock(idbBlock);

    for (int i = 0; i < 256; i++)
    {
        dataBlockNum = idb->dataAddresses[i];
        if (dataBlockNum == -1 || dataBlockNum == 0)
        {
            if (bytesLeft <= 0)
            {
                if (debug)
                    printf("finished reading in content from disk in the indirection block %d\n", i);
                break;
            }
            else
            {
                printf("Reached a null block pointer (-1) but according to the file size\n");
                printf("of the inode there should be %d bytes of data to read, therefore something\n", bytesLeft);
                printf("must be corrupted, returning NULL\n");
                return NULL;
            }
        }

        readBlock(disk, dataBlockNum, data + offset);
        offset += BLOCK_SIZE;
        bytesLeft -= BLOCK_SIZE;
    }

    if (bytesLeft >= 0)
    {
        printf("finished reading data from the indirection block but\n");
        printf("there's %d more bytes of data to read according to the file size of inode\n", bytesLeft);
        printf("something must be corrupted, returning NULL");
        return NULL;
    }

    free(idbBlock);
    data[inode->sizeOfFile] = '\0';
    return data;
}

void deleteInodeFile(FILE *disk, struct inode *inode)
{
    if (inode->flags != REGFILE)
    {
        printf("called delete inode file with an inode that doesn't contain a file, returning without deleting anything\n");
        return;
    }

    if (debug)
        printf("deleting contents of an inode, %d bytes\n", inode->sizeOfFile);
    int bytesDeleted = 0;
    short blockNum;
    char *emptyBlock = calloc(1, BLOCK_SIZE);

    //Delete file contents from the first 10 addresses in the inode
    for (int i = 0; i < 10; i++)
    {
        blockNum = inode->dataAddresses[i];
        if (blockNum == -1)
        {
            if (debug)
                printf("reached a null block pointer in the inode, file contents were deleted\n");
            break;
        }
        if (debug)
            printf("deleting block %d from disk\n", blockNum);
        writeBlock(disk, blockNum, emptyBlock, BLOCK_SIZE);
        inode->dataAddresses[i] = -1;
        markBlockAvailable(disk, blockNum);
        bytesDeleted += BLOCK_SIZE;
    }

    //if there's no indirection block it means we've deleted everything
    if (inode->singleIndirectBlock == -1)
    {
        if (debug)
            printf("no indirection block to delete from, done deleting stuff from disk!\n");
        inode->flags = AVAILABLE;
        inode->sizeOfFile = 0;
        inode->flags = 0;
        free(emptyBlock);
        return;
    }

    //load in the inodes indirection block
    char *idbBlock = malloc(BLOCK_SIZE);
    readBlock(disk, inode->singleIndirectBlock, idbBlock);
    struct indirectionBlock *idb = bytesToIndirectionBlock(idbBlock);

    //delete any contents the indirection block points to
    for (int i = 0; i < 256; i++)
    {
        blockNum = idb->dataAddresses[i];
        if (blockNum == -1)
        {
            if (debug)
                printf("reached a null block pointer in the indirection block, file contents were deleted\n");
            break;
        }
        if (debug)
            printf("deleting block %d from disk\n", blockNum);
        writeBlock(disk, blockNum, emptyBlock, BLOCK_SIZE);
        idb->dataAddresses[i] = -1;
        markBlockAvailable(disk, blockNum);
        bytesDeleted += BLOCK_SIZE;
    }

    //delete indirection block
    if (debug)
        printf("deleting the indirection block from disk on block %d\n", inode->singleIndirectBlock);
    writeBlock(disk, inode->singleIndirectBlock, emptyBlock, BLOCK_SIZE);
    markBlockAvailable(disk, inode->singleIndirectBlock);
    inode->singleIndirectBlock = -1;
    inode->flags = AVAILABLE;
    inode->sizeOfFile = 0;
    inode->flags = 0;

    if (debug)
        printf("done deleting stuff from disk, inode is now clean after deleting %d bytes\n", bytesDeleted);
    free(emptyBlock);
    free(idbBlock);
}

//delete directory from inode, mark block available, mark inode Available, get rid of inode pointer to directory
void deleteInodeDirectory(FILE *disk, struct inode *inode)
{
    if (debug)
        printf("Deleting directory from inode, Meaning everything in it must have been deleted as well\n");
    if (inode->flags != DIRECTORY)
    {
        printf("The inode is not for a directory, doing nothing\n");
        return;
    }

    //erase directory
    char *emptyBlock = calloc(1, BLOCK_SIZE);
    int directoryBlockNum = inode->dataAddresses[0]; //by convention
    writeBlock(disk, directoryBlockNum, emptyBlock, BLOCK_SIZE);
    inode->dataAddresses[0] = -1;
    inode->flags = 0;
    markBlockAvailable(disk, directoryBlockNum);

    inode->flags = AVAILABLE;
    free(emptyBlock);
}

//create superblock: first 4 bytes "magic number", next 4 number of
//blocks on disk, next 4 number of inodes on disk
char *createSuperblock()
{
    char *superblock = calloc(1, BLOCK_SIZE);
    int magic = 37;
    int numBlocks = NUM_BLOCKS;
    int numInodes = NUM_INODE;
    memcpy(superblock, &magic, 4);
    memcpy(superblock + 4, &numBlocks, 4);
    memcpy(superblock + 8, &numInodes, 4);
    return superblock;
}

//create a chunk of memory with 4096 bits
//each representing a block in the file system
int *createFreeBlockList()
{
    int *freeBlockList = (int *)malloc(BLOCK_SIZE);

    //first 10 blocks can't store data, so they're 0
    for (int i = 0; i < 10; i++)
    {
        ClearBit(freeBlockList, i);
    }

    //the rest can so they're 1
    for (int i = 11; i < NUM_BLOCKS; i++)
    {
        SetBit(freeBlockList, i);
    }
    return freeBlockList;
}

// //create a block with 8 fresh inodes within it
// char *createBlockOfInodes()
// {
//     char *newInode = inodeToBytes(createEmptyInode());
//     char *newBlock = malloc(BLOCK_SIZE);

//     int offset = 0;
//     for (int i = 0; i < BLOCK_SIZE / INODE_SIZE; i++)
//     {
//         memcpy(newBlock + offset, newInode, INODE_SIZE);
//         offset += INODE_SIZE;
//     }
//     return newBlock;
// }

//Create inodes for the file system, 8 blocks totalling 128 inodes
char *createInodes()
{
    char *inodes = malloc(BLOCK_SIZE * 8);
    int blockNumberOffset = INODESTART * BLOCK_SIZE;
    struct inode *referenceInode = createEmptyInode();

    //each inode is 32 bytes, each block is 512 bytes
    //In this file system theres 8 blocks of Inodes ie. 128 of them
    for (int offset = 0; offset <= BLOCK_SIZE * 8; offset += 32)
    {
        referenceInode->location = offset + blockNumberOffset;
        memcpy(inodes + offset, inodeToBytes(referenceInode), INODE_SIZE);
    }

    return inodes;
}

//0: superblock
//1: freeblock List
//2 - 10: Inodes
//11: root directory
//12 - 4095: free blocks
void formatDisk(FILE *disk)
{
    //initialize the disk to all zero's with all the memory it should have
    char *emptyblock = calloc(1, BLOCK_SIZE);
    for (int i = 0; i < NUM_BLOCKS; i++)
    {
        writeBlock(disk, i, emptyblock, BLOCK_SIZE);
    }

    //create the superblock
    char *superblock = createSuperblock();
    writeBlock(disk, 0, superblock, BLOCK_SIZE);

    //create the freeblock list
    int *freeBlockList = createFreeBlockList();
    writeBlock(disk, 1, (char *)freeBlockList, BLOCK_SIZE);

    //create inodes
    char *inodes = createInodes();
    for (int blockNumber = INODESTART; blockNumber <= INODEEND; blockNumber++)
    {
        writeBlock(disk, blockNumber, inodes + ((blockNumber - INODESTART) * BLOCK_SIZE), BLOCK_SIZE);
    }

    //find a free node, so it can be used to point to the root directory
    struct inode *inode = findFreeInode(disk);

    //modify inode accordingly and update vdisk
    inode->dataAddresses[0] = ROOTDIR;
    inode->flags = DIRECTORY;
    updateInode(disk, inode);

    //create an empty directory and write it to vdisk
    struct directory *dir = createEmptyDirectory();
    char *dirData = directoryToBytes(dir);
    markBlockUnavailable(disk, ROOTDIR);
    writeBlock(disk, ROOTDIR, dirData, BLOCK_SIZE);

    free(superblock);
    free(freeBlockList);
    free(emptyblock);
    free(inodes);
    free(inode);
    free(dir);
    free(dirData);
}

//TO-DO make free functions for all the structs