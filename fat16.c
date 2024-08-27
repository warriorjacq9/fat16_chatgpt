#include "fat16.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// Global variables
FAT16BootSector bootSector;
uint16_t rootDirStartSector;
uint16_t fatStartSector;
uint16_t dataStartSector;

// Read a sector from the disk
void readSector(uint32_t sector, void* buffer) {
    // Implement disk read here
    // Example: disk_read(sector * SECTOR_SIZE, buffer, SECTOR_SIZE);
}

// Write a sector to the disk
void writeSector(uint32_t sector, const void* buffer) {
    // Implement disk write here
    // Example: disk_write(sector * SECTOR_SIZE, buffer, SECTOR_SIZE);
}

// Read the boot sector
void readBootSector() {
    readSector(0, &bootSector);
    fatStartSector = bootSector.reservedSectors;
    dataStartSector = fatStartSector + (bootSector.fatCount * bootSector.fatSize16);
    rootDirStartSector = dataStartSector;
}

// Read the FAT table
void readFAT(uint8_t* fat) {
    for (int i = 0; i < bootSector.fatSize16; i++) {
        readSector(fatStartSector + i, fat + i * SECTOR_SIZE);
    }
}

// Read a directory
void readDirectory(uint16_t startCluster, FAT16DirEntry* entries, uint16_t* count) {
    uint16_t cluster = startCluster;
    uint16_t entryCount = 0;
    uint8_t sectorBuffer[SECTOR_SIZE];

    while (cluster < FAT16_MAX_CLUSTER && entryCount < ROOT_DIR_ENTRIES) {
        uint32_t sectorOffset = 0;
        uint32_t sector = dataStartSector + (cluster - 2) * bootSector.sectorsPerCluster;

        for (int i = 0; i < bootSector.sectorsPerCluster; i++) {
            readSector(sector + i, sectorBuffer);
            for (int j = 0; j < SECTOR_SIZE / sizeof(FAT16DirEntry); j++) {
                if (entryCount >= ROOT_DIR_ENTRIES) {
                    return;
                }
                memcpy(&entries[entryCount++], sectorBuffer + sectorOffset, sizeof(FAT16DirEntry));
                sectorOffset += sizeof(FAT16DirEntry);
            }
        }
        cluster = getNextCluster(cluster);
    }
    *count = entryCount;
}

// Get the next cluster in the FAT chain
uint16_t getNextCluster(uint16_t cluster) {
    uint8_t fat[SECTOR_SIZE];
    readFAT(fat);
    uint16_t offset = cluster * 2;
    uint16_t entry = (fat[offset + 1] << 8) | fat[offset];
    return entry;
}

// Find a free file descriptor slot
static FileDescriptor* getFreeFileDescriptor() {
    static FileDescriptor openFiles[MAX_OPEN_FILES] = {0}; // Initialize to zeros
    for (int i = 0; i < MAX_OPEN_FILES; i++) {
        if (!openFiles[i].isOpen) {
            return &openFiles[i];
        }
    }
    return NULL;
}

// Check if a directory entry is a long filename entry
bool isLongFilenameEntry(const FAT16DirEntry* entry) {
    return (entry->attributes & 0x0F) == 0x0F;
}

// Get the long filename from directory entries
void getLongFilename(FAT16DirEntry* entries, uint16_t count, char* longFilename) {
    longFilename[0] = '\0';
    for (int i = 0; i < count; ++i) {
        if (isLongFilenameEntry(&entries[i])) {
            FAT16LFNEntry* lfnEntry = (FAT16LFNEntry*)&entries[i];
            uint8_t seq = lfnEntry->order & 0x3F;
            if (seq == 0) {
                strncat(longFilename, (char*)lfnEntry->name1, 10);
                strncat(longFilename, (char*)lfnEntry->name2, 12);
                strncat(longFilename, (char*)lfnEntry->name3, 4);
            } else {
                strncat(longFilename, (char*)lfnEntry->name1, 5);
                strncat(longFilename, (char*)lfnEntry->name2, 6);
                strncat(longFilename, (char*)lfnEntry->name3, 2);
            }
        }
    }
}

// Parse a path into directory and base file names
bool parsePath(const char* path, char* dirName, char* baseName) {
    char* lastSlash = strrchr(path, '/');
    if (lastSlash) {
        strncpy(dirName, path, lastSlash - path);
        dirName[lastSlash - path] = '\0';
        strcpy(baseName, lastSlash + 1);
    } else {
        dirName[0] = '\0';
        strcpy(baseName, path);
    }
    return true;
}

// Find a file or directory entry
void findFileEntry(const char* path, FAT16DirEntry* entry, uint16_t* parentCluster) {
    char dirName[FILE_NAME_LENGTH];
    char baseName[FILE_NAME_LENGTH];
    parsePath(path, dirName, baseName);

    uint16_t currentCluster = 0;
    FAT16DirEntry entries[ROOT_DIR_ENTRIES];
    uint16_t entryCount = 0;

    if (strlen(dirName) == 0) {
        currentCluster = 0; // Root directory
    } else {
        // Traverse directories to find the target directory
        findFileEntry(dirName, NULL, &currentCluster);
        if (currentCluster == 0) {
            memset(entry, 0, sizeof(FAT16DirEntry));
            return;
        }
    }

    readDirectory(currentCluster, entries, &entryCount);

    for (int i = 0; i < entryCount; i++) {
        FAT16DirEntry* dirEntry = &entries[i];

        if (isLongFilenameEntry(dirEntry)) {
            // Skip long filename entries
            continue;
        }

        if (memcmp(dirEntry->filename, baseName, 8) == 0) {
            *entry = *dirEntry;
            *parentCluster = currentCluster;
            return;
        }
    }

    memset(entry, 0, sizeof(FAT16DirEntry));
}

// Open a file or directory
FileDescriptor* openFile(const char* path, const char* mode) {
    FAT16DirEntry entry;
    uint16_t parentCluster;
    findFileEntry(path, &entry, &parentCluster);

    if (entry.fileSize == 0 && memcmp(&entry, "\0", sizeof(FAT16DirEntry)) == 0) {
        return NULL;  // File or directory not found
    }

    FileDescriptor* fd = getFreeFileDescriptor();
    if (!fd) {
        return NULL;  // No free file descriptors
    }

    // Initialize file descriptor
    fd->firstCluster = entry.firstClusterLow | (entry.firstClusterHigh << 16);
    fd->fileSize = entry.fileSize;
    fd->currentOffset = 0;
    fd->isOpen = true;
    fd->isDirectory = (entry.attributes & 0x10) != 0; // Check if it is a directory
    fd->parentCluster = parentCluster;

    // Handle long filenames
    if (isLongFilenameEntry(&entry)) {
        // Rebuild the long filename from LFN entries
        FAT16DirEntry entries[ROOT_DIR_ENTRIES];
        readDirectory(parentCluster, entries, NULL);
        getLongFilename(entries, ROOT_DIR_ENTRIES, fd->longFilename);
        strcpy(fd->filename, fd->longFilename);
    } else {
        // Use short filename
        snprintf(fd->filename, FILE_NAME_LENGTH, "%s.%s", entry.filename, entry.ext);
    }

    return fd;
}

// Write to a file
size_t writeFile(FileDescriptor* fd, const void* buffer, size_t size) {
    if (fd->isDirectory) {
        return 0; // Cannot write to a directory
    }

    uint32_t offset = fd->currentOffset;
    size_t written = 0;
    uint8_t sectorBuffer[SECTOR_SIZE];
    uint16_t cluster = fd->firstCluster;

    while (size > 0) {
        uint32_t sectorOffset = offset % SECTOR_SIZE;
        uint32_t bytesToWrite = SECTOR_SIZE - sectorOffset;
        if (bytesToWrite > size) {
            bytesToWrite = size;
        }

        // Read the current sector into buffer if writing to existing file
        readSector(dataStartSector + (cluster - 2) * bootSector.sectorsPerCluster + (offset / SECTOR_SIZE), sectorBuffer);

        // Write data to the sector buffer
        memcpy(sectorBuffer + sectorOffset, buffer, bytesToWrite);

        // Write the modified sector back to the disk
        writeSector(dataStartSector + (cluster - 2) * bootSector.sectorsPerCluster + (offset / SECTOR_SIZE), sectorBuffer);

        buffer = (const char*)buffer + bytesToWrite;
        size -= bytesToWrite;
        written += bytesToWrite;
        offset += bytesToWrite;

        if (offset >= fd->fileSize) {
            break;  // End of file
        }

        if (size > 0) {
            cluster = getNextCluster(cluster);
            if (cluster >= FAT16_MAX_CLUSTER) {
                return written;  // End of file or error
            }
            offset = 0;
        }
    }

    fd->currentOffset += written;
    return written;
}

// Read from a file
size_t readFile(FileDescriptor* fd, void* buffer, size_t size) {
    if (fd->isDirectory) {
        return 0; // Cannot read from a directory
    }

    uint32_t offset = fd->currentOffset;
    size_t read = 0;
    uint8_t sectorBuffer[SECTOR_SIZE];
    uint16_t cluster = fd->firstCluster;

    while (size > 0) {
        uint32_t sectorOffset = offset % SECTOR_SIZE;
        uint32_t bytesToRead = SECTOR_SIZE - sectorOffset;
        if (bytesToRead > size) {
            bytesToRead = size;
        }

        // Read the current sector into buffer
        readSector(dataStartSector + (cluster - 2) * bootSector.sectorsPerCluster + (offset / SECTOR_SIZE), sectorBuffer);

        // Copy data from the sector buffer
        memcpy(buffer, sectorBuffer + sectorOffset, bytesToRead);

        buffer = (char*)buffer + bytesToRead;
        size -= bytesToRead;
        read += bytesToRead;
        offset += bytesToRead;

        if (offset >= fd->fileSize) {
            break;  // End of file
        }

        if (size > 0) {
            cluster = getNextCluster(cluster);
            if (cluster >= FAT16_MAX_CLUSTER) {
                return read;  // End of file or error
            }
            offset = 0;
        }
    }

    fd->currentOffset += read;
    return read;
}

// Close an open file
int closeFile(FileDescriptor* fd) {
    if (!fd->isOpen) {
        return -1;  // File not open
    }

    fd->isOpen = false;
    return 0;
}
