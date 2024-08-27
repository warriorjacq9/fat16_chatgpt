#ifndef FAT16_H
#define FAT16_H

#include <stdint.h>
#include <stdbool.h>

#define SECTOR_SIZE 512
#define ROOT_DIR_ENTRIES 512
#define FILE_NAME_LENGTH 13
#define MAX_LONG_FILENAME_LENGTH 256
#define MAX_OPEN_FILES 10
#define FAT16_MAX_CLUSTER 0xFFF5

// FAT16 Boot Sector structure
typedef struct {
    uint8_t  jmp[3];
    uint8_t  oem[8];
    uint16_t bytesPerSector;
    uint8_t  sectorsPerCluster;
    uint16_t reservedSectors;
    uint8_t  fatCount;
    uint16_t rootDirEntries;
    uint16_t totalSectors16;
    uint8_t  mediaDescriptor;
    uint16_t fatSize16;
    uint16_t sectorsPerTrack;
    uint16_t heads;
    uint32_t hiddenSectors;
    uint32_t totalSectors32;
} __attribute__((packed)) FAT16BootSector;

// FAT16 Directory Entry structure
typedef struct {
    uint8_t  filename[8];
    uint8_t  ext[3];
    uint8_t  attributes;
    uint8_t  reserved;
    uint8_t  creationTimeTenths;
    uint16_t creationTime;
    uint16_t creationDate;
    uint16_t lastAccessDate;
    uint16_t firstClusterHigh;
    uint16_t lastWriteTime;
    uint16_t lastWriteDate;
    uint16_t firstClusterLow;
    uint32_t fileSize;
} __attribute__((packed)) FAT16DirEntry;

// FAT16 Long Filename Entry structure
typedef struct {
    uint8_t  order;
    uint16_t name1[5];      // First 5 UTF-16 characters
    uint8_t  attributes;
    uint8_t  type;
    uint8_t  checksum;
    uint16_t name2[6];      // Next 6 UTF-16 characters
    uint16_t reserved;
    uint16_t name3[2];      // Last 2 UTF-16 characters
} __attribute__((packed)) FAT16LFNEntry;

// File descriptor structure
typedef struct {
    char filename[FILE_NAME_LENGTH];
    char longFilename[MAX_LONG_FILENAME_LENGTH];
    uint16_t firstCluster;
    uint32_t fileSize;
    uint32_t currentOffset;
    bool isOpen;
    bool isDirectory;
    uint16_t parentCluster;
} FileDescriptor;

// Function prototypes
void readSector(uint32_t sector, void* buffer);
void writeSector(uint32_t sector, const void* buffer);
void readBootSector();
void readFAT(uint8_t* fat);
void readDirectory(uint16_t startCluster, FAT16DirEntry* entries, uint16_t* count);
uint16_t getNextCluster(uint16_t cluster);
FileDescriptor* getFreeFileDescriptor();
void findFileEntry(const char* path, FAT16DirEntry* entry, uint16_t* parentCluster);
FileDescriptor* openFile(const char* path, const char* mode);
size_t writeFile(FileDescriptor* fd, const void* buffer, size_t size);
size_t readFile(FileDescriptor* fd, void* buffer, size_t size);
int closeFile(FileDescriptor* fd);

#endif // FAT16_H
