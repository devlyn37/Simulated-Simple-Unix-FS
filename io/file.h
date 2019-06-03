#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

//inode flags
const int DIRECTORY;
const int REGFILE;
const int AVAILABLE;

//blocks
const int SUPERBLOCK;
const int FREEBLOCKLIST;
const int INODESTART;
const int INODEEND;
const int ROOTDIR;
const int FREESTART;
const int FREEEND;

struct inode;
struct directoryEntry;
struct directory;
struct indirectionBlock;

struct inode *createInode(int sizeOfFile, int flags, short *dataAddresses, short singleIndirectBlock, short doubleIndirectBlock);
struct inode *createEmptyInode();
struct inode *bytesToInode(char *data);
struct inode *findFreeInode(FILE *disk);
char *inodeToBytes(struct inode *inode);
void printInode(struct inode *inode);
void updateInode(FILE *disk, struct inode *inode);
struct inode *getInode(FILE *disk, short address);
void writeToInode(FILE *disk, struct inode *inode, char *data);
void addDirToInode(FILE *disk, struct inode *inode, struct directory *dir);
struct inode *getRootInode(FILE *disk);
char *readInode(FILE *disk, struct inode *inode);
void deleteInodeFile(FILE *disk, struct inode *inode);
void deleteInodeDirectory(FILE *disk, struct inode *inode);

struct directoryEntry *createDirectoryEntry(short available, short address, char *name);
struct directoryEntry *createEmptyDirectoryEntry();
struct directoryEntry *BytesToDirectoryEntry(char *data);
struct directoryEntry *findDirectoryEntry(struct directory *dir, char *name);
struct directoryEntry *findEmptyDirectoryEntry(struct directory *dir);
char *directoryEntryToBytes(struct directoryEntry *entry);

struct directory *createDirectory(struct directoryEntry **entries);
struct directory *createEmptyDirectory();
struct directory *bytesToDirectory(char *data);
struct directory *getRootDir(FILE *disk);
struct directory *getDirectory(FILE *disk, struct inode *inode);
char *directoryToBytes(struct directory *dir);
void printDirectory(struct directory *dir);
void updateDir(FILE *disk, struct inode *inode, struct directory *dir);

struct indirectionBlock *createIndirectionBlock(short *addresses);
struct indirectionBlock *createEmptyIndirectionBlock();
struct indirectionBlock *bytesToIndirectionBlock(char *data);
char *indirectionBlockToBytes(struct indirectionBlock *idb);
void printIndirectionBlock(struct indirectionBlock *idb);

int findFreeBlock(FILE *disk);
void markBlockAvailable(FILE *disk, int blockNum);
void unmarkBlockUnavailable(FILE *disk, int blockNum);

void formatDisk();