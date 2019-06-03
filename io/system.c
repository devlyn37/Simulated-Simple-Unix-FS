#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "FsStructs.h"
#include "file.h"

const int DIRDEPTHLIMIT = 10;
const int NAMELENGTHLIMIT = 35;
const char *SimulatedDisk = "disk/vdisk";
int debugS = 0;

//looked at stack overflow for the best way to load file into buffer
//https://stackoverflow.com/questions/174531/how-to-read-the-content-of-a-file-to-a-string-in-c
char *fileToBytes(char *fileLocation)
{
    char *buffer = 0;
    long length;
    FILE *newFile = fopen(fileLocation, "r");

    if (newFile)
    {
        if (debugS)
            printf("trying to copy over the file into memory\n");
        int seekResult = fseek(newFile, 0, SEEK_END);
        if (seekResult != 0)
        {
            printf("seek error\n");
            fclose(newFile);
            return "";
        }

        length = ftell(newFile);
        if (length < 0)
        {
            printf("ftell error\n");
            fclose(newFile);
            return "";
        }

        seekResult = fseek(newFile, 0, SEEK_SET);
        if (seekResult != 0)
        {
            printf("seek error\n");
            fclose(newFile);
            return "";
        }

        buffer = malloc(length + 1);
        if (buffer)
        {
            fread(buffer, 1, length, newFile);
        }
        fclose(newFile);
        buffer[length] = '\0';
    }

    return buffer;
}

//https : //www.codingame.com/playgrounds/14213/how-to-play-with-strings-in-c/string-split
//creates an array of strings, with one for each directory in the path
//the size limit of the array is 10
char **interpretPath(char *path)
{
    //strtok modifies the input string, so you can't pass in string literals
    //because that behavior is undefined
    //this codes here because string literals are nice to test with
    char *str = malloc(strlen(path) + 1);
    strcpy(str, path);
    str[strlen(path)] = '\0';
    //printf("%lu, %s\n", strlen(str), str);

    char **pathArr = malloc(sizeof(char *) * DIRDEPTHLIMIT);
    for (int i = 0; i < DIRDEPTHLIMIT; i++)
        pathArr[i] = "";

    char *token;
    int i = 0;

    token = strtok(str, "/");
    while (token != NULL)
    {
        pathArr[i++] = token;
        //printf("%lu, %s\n", strlen(token), token);
        if (i > DIRDEPTHLIMIT)
        {
            printf("The limit for directories is %d, stopping at %s\n", DIRDEPTHLIMIT, token);
            break;
        }
        token = strtok(NULL, "/");
    }

    return pathArr;
}

//given a path return the correct inode for the directory
struct inode *traversePath(FILE *disk, char *path)
{
    char **pathParts = interpretPath(path);
    char *desiredDirName;

    if (debugS)
        printf("in Traverse path!\n");
    for (int i = 0; i < 10; i++)
    {
        if (debugS)
            printf("%s ", pathParts[i]);
    }
    if (debugS)
        printf("\n");

    struct directory *root = getRootDir(disk);
    struct directory *currentDir = root;

    struct directoryEntry *currentEntry;
    struct inode *desiredDirInode = getRootInode(disk);

    //There can be a maximum directory depth of 10, an empty string in the path parts
    //represents the end of the path if its less than 10 directories deep
    for (int i = 0; i < DIRDEPTHLIMIT && strlen(pathParts[i]) != 0; i++)
    {
        desiredDirName = pathParts[i];
        if (debugS)
            printf("On the number %d directory trying to get into the path %s\n", i, desiredDirName);
        if (strlen(desiredDirName) == 0)
        {
            if (debugS)
                printf("Reached desired directory!\n");
            break;
        }

        currentEntry = findDirectoryEntry(currentDir, desiredDirName);
        if (currentEntry == NULL)
        {
            printf("Something went wrong when grabbing entry, returning null\n");
            return NULL;
        }
        if (debugS)
            printf("Just grabbed entry for the directory %s\n", currentEntry->name);

        desiredDirInode = getInode(disk, currentEntry->address);
        if (debugS)
            printf("just grabbed inode for that directory, block number of directory = %d\n", desiredDirInode->dataAddresses[0]);

        currentDir = getDirectory(disk, desiredDirInode);
        if (currentDir == NULL)
        {
            printf("something went wrong when grabbing the directory, returning null\n");
            return NULL;
        }
        if (debugS)
            printf("Just grabbed the directory, Yay!\n");
    }

    return desiredDirInode;
}

void deleteFile(char *path, char *name)
{
    FILE *disk = fopen(SimulatedDisk, "rb+");

    struct inode *parentDirInode = traversePath(disk, path);
    struct directory *parentDir = getDirectory(disk, parentDirInode);
    struct directoryEntry *dirEntryForFile = findDirectoryEntry(parentDir, name);

    if (dirEntryForFile == NULL)
    {
        printf("could not find file %s in directory %s, returning without deleting anything\n", name, path);
        return;
    }

    short fileInodeAddress = dirEntryForFile->address;
    struct inode *fileInode = getInode(disk, fileInodeAddress);

    if (fileInode->flags != REGFILE)
    {
        printf("This is a directory or it hasnt been initialized returning without doing anything\n");
        return;
    }

    markBlockAvailable(disk, dirEntryForFile->address);
    dirEntryForFile->address = -1;
    dirEntryForFile->available = 1;
    dirEntryForFile->name = "";

    deleteInodeFile(disk, fileInode);
    updateInode(disk, fileInode);
    updateDir(disk, parentDirInode, parentDir);
    fclose(disk);
    printf("done deleting file :)\n");
}

//look at this later, inode address might not be needed
//kk
//kk
//kk
void deleteDirectoryRecursive(FILE *disk, struct inode *inode, int inodeAddress)
{
    //basecase regular file or empty directory
    if (inode->flags == REGFILE)
    {
        if (debugS)
            printf("deleting file\n");
        deleteInodeFile(disk, inode);
        updateInode(disk, inode);
        return;
    }

    if (inode->flags != DIRECTORY)
    {
        printf("Inode hasn't been properly initialized, returning\n");
        return;
    }

    struct directory *directory = getDirectory(disk, inode);

    struct directoryEntry *entry;
    short entryInodeAddress;
    struct inode *entryInode;

    if (debugS)
        printf("going through each entry of this directory\n");
    for (int i = 0; i < 16; i++)
    {
        entry = directory->entries[i];
        //if every entry has null pointers no more recursion will happen
        if (entry->address != -1)
        {
            if (debugS)
                printf("On directory or file named %s\n", entry->name);
            entryInodeAddress = entry->address;
            entryInode = getInode(disk, entryInodeAddress);
            deleteDirectoryRecursive(disk, entryInode, entryInodeAddress);
        }
    }

    if (debugS)
        printf("finished going through each entry in the directory and deleting it\n");
    if (debugS)
        printf("now deleting directory\n");
    deleteInodeDirectory(disk, inode);
    updateInode(disk, inode);
}

void deleteDirectory(char *path, char *name)
{
    FILE *disk = fopen(SimulatedDisk, "rb+");

    struct inode *parentDirInode = traversePath(disk, path);
    struct directory *parentDir = getDirectory(disk, parentDirInode);
    struct directoryEntry *dirEntryForDirectory = findDirectoryEntry(parentDir, name);

    if (dirEntryForDirectory == NULL)
    {
        printf("could not find directory %s in directory %s, returning without deleting anything\n", name, path);
        return;
    }

    short directoryInodeAddress = dirEntryForDirectory->address;
    struct inode *directoryInode = getInode(disk, directoryInodeAddress);

    if (directoryInode->flags != DIRECTORY)
    {
        printf("This is a regular file or it hasnt been initialized, returning without doing anything\n");
        return;
    }

    markBlockAvailable(disk, dirEntryForDirectory->address);
    dirEntryForDirectory->address = -1;
    dirEntryForDirectory->available = 1;
    dirEntryForDirectory->name = "";

    deleteDirectoryRecursive(disk, directoryInode, directoryInodeAddress);
    updateDir(disk, parentDirInode, parentDir);
    fclose(disk);
    printf("done deleting directory :)\n");
}

char *readFile(char *path, char *name)
{
    FILE *disk = fopen(SimulatedDisk, "rb+");

    struct inode *parentDirInode = traversePath(disk, path);
    struct directory *parentDir = getDirectory(disk, parentDirInode);
    struct directoryEntry *dirEntryForFile = findDirectoryEntry(parentDir, name);

    if (dirEntryForFile == NULL)
    {
        printf("could not find file %s in directory %s, returning NULL\n", name, path);
        return NULL;
    }

    struct inode *fileInode = getInode(disk, dirEntryForFile->address);
    char *fileContents = readInode(disk, fileInode);

    if (fileContents == NULL)
    {
        printf("Something went wrong when reading the file, returning NULL\n");
    }

    fclose(disk);
    return fileContents;
}

void mkDir(char *path, char *name)
{
    FILE *disk = fopen(SimulatedDisk, "rb+");

    struct inode *parentDirInode = traversePath(disk, path);
    struct directory *parentDir = getDirectory(disk, parentDirInode);
    //printf("parent directory inode:\n");
    //printInode(parentDirInode);
    if (parentDir == NULL)
    {
        printf("something went wrong when grabbing parent directory\n");
        return;
    }
    //printf("Parent directory before:\n");
    //printDirectory(parentDir);

    struct directoryEntry *newDirEntry = findEmptyDirectoryEntry(parentDir);
    if (newDirEntry == NULL)
    {
        printf("Not enough room in the parent directory for a new one, returning\n");
        return;
    }
    struct directory *newDir = createEmptyDirectory();
    struct inode *newDirInode = findFreeInode(disk);

    //set up new entry in parent directory
    newDirEntry->address = newDirInode->location;
    newDirEntry->available = 0;
    newDirEntry->name = name;

    //set up new inode
    newDirInode->flags = DIRECTORY;

    if (debugS)
        printf("writing data from new directory to its inode\n");
    addDirToInode(disk, newDirInode, newDir);
    if (debugS)
        printf("updating inode\n");
    updateInode(disk, newDirInode);

    if (debugS)
        printf("updating the parent directory\n");
    // printf("parent directory after:\n");
    //printDirectory(parentDir);
    updateDir(disk, parentDirInode, parentDir);
    fclose(disk);

    //free everything
    free(newDir);
    free(newDirInode);
    free(parentDir);
    free(parentDirInode);
    printf("done making directory :)\n");
}

void mkFile(char *fileLocation, char *path, char *name)
{
    char *fileContents = fileToBytes(fileLocation);
    //printf("file contents:\n %s\n", fileContents);

    FILE *disk = fopen(SimulatedDisk, "rb+");

    //traverse to and get directory from path
    struct inode *currentDirInode = traversePath(disk, path);
    struct directory *currentDir = getDirectory(disk, currentDirInode);
    //printf("adding new file to directory %s:\n", path);
    //printDirectory(currentDir);

    //grab an available entry in the directory;
    struct directoryEntry *availbleEntry = findEmptyDirectoryEntry(currentDir);

    //grab an inode for the new file
    //printf("ref id: %d\n", ref->id);
    struct inode *inodeForNewFile = findFreeInode(disk);
    if (inodeForNewFile == NULL)
    {
        printf("No available free inodes\n");
        return;
    }
    inodeForNewFile->flags = REGFILE;
    inodeForNewFile->sizeOfFile = strlen(fileContents);

    //write data to the various blocks the inode references
    writeToInode(disk, inodeForNewFile, fileContents);
    //write changes to inode into disk
    updateInode(disk, inodeForNewFile);

    //update directory with an entry for the file
    //point entry address to the inode for the new file
    availbleEntry->address = inodeForNewFile->location;
    availbleEntry->available = 0;
    //entry name matches the name for the file provided in the parameters;
    availbleEntry->name = name;

    //write changes to directory back to disk
    //printf("writing directory to disk!\n");
    updateDir(disk, currentDirInode, currentDir);
    //printf("current directory afterwards:\n");
    //printDirectory(currentDir);

    free(fileContents);
    fclose(disk);
    printf("Done adding file :)\n");
}

void initializeSimulatedDisk()
{
    FILE *disk = fopen(SimulatedDisk, "rb+");
    formatDisk(disk);
    fclose(disk);
}

void fileSystemOverview()
{
    //first look at the inodes
    FILE *disk = fopen(SimulatedDisk, "rb+");
    struct inode *currentInode;

    int dirInodeCount = 0;
    int fileInodeCount = 0;
    int inactiveinodeCount = 0;
    for (int i = 512 * INODESTART; i < 512 * INODEEND; i += 32)
    {
        currentInode = getInode(disk, i);
        if (currentInode->flags == 0)
        {
            inactiveinodeCount++;
        }
        else if (currentInode->flags == REGFILE)
        {
            fileInodeCount++;
        }
        else if (currentInode->flags == DIRECTORY)
        {
            dirInodeCount++;
        }
    }

    printf("Current File System Status: %d directories, %d files, %d inactive inodes\n", dirInodeCount, fileInodeCount, inactiveinodeCount);
    fclose(disk);
}