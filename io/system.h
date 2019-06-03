const int DIRDEPTHLIMIT;
const int NAMELENGTHLIMIT;
const char *SimulatedDisk;

void deleteFile(char *path, char *name);
void deleteDirectory(char *path, char *name);
char *readFile(char *path, char *name);
void mkDir(char *path, char *name);
void mkFile(char *fileLocation, char *path, char *name);
void initializeSimulatedDisk();
void fileSystemOverview();
