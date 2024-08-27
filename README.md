# FAT16 Driver

A FAT16 file system driver written in C. This driver allows you to interact with FAT16 formatted disks, supporting basic file operations such as reading, writing, and directory management.

## Features

- **Read and Write Files:** Basic operations to read from and write to files on a FAT16 file system.
- **Directory Management:** Support for navigating and listing directories, including handling subdirectories.
- **Long Filename Support:** Proper handling of long filenames as specified in the FAT16 file system.
- **File Descriptor Management:** Management of file descriptors for tracking open files.

## Dependencies

- **Standard C Library:** Required for functions like `memcpy`, `strlen`, `strcpy`, `strncat`, `strrchr`, `snprintf`, and `memcmp`.
- **Platform-Specific Disk I/O:** Implement the `readSector` and `writeSector` functions according to your platform or hardware.
- **Build System:** CMake, Makefile, or other build systems for compiling the driver.

## Installation

1. **Clone the Repository:**

    ```bash
    git clone https://github.com/yourusername/fat16-driver.git
    cd fat16-driver
    ```

2. **Build the Driver:**

    Depending on your build system, you might use one of the following commands:

    - **Using Makefile:**

        ```bash
        make
        ```

    - **Using CMake:**

        ```bash
        mkdir build
        cd build
        cmake ..
        make
        ```

    Ensure that the `readSector` and `writeSector` functions are properly implemented for your environment before building.

## Usage

1. **Include the Header File:**

    Include the `fat16.h` header file in your source code to use the driver functions.

    ```c
    #include "fat16.h"
    ```

2. **Initialize the File System:**

    Call `readBootSector()` to initialize the file system.

    ```c
    readBootSector();
    ```

3. **Open a File:**

    ```c
    FileDescriptor* fd = openFile("path/to/file.txt", "r");
    if (fd) {
        // File opened successfully
    } else {
        // Handle error
    }
    ```

4. **Read from a File:**

    ```c
    char buffer[128];
    size_t bytesRead = readFile(fd, buffer, sizeof(buffer));
    ```

5. **Write to a File:**

    ```c
    const char* data = "Hello, FAT16!";
    size_t bytesWritten = writeFile(fd, data, strlen(data));
    ```

6. **Close the File:**

    ```c
    closeFile(fd);
    ```

## Examples

For detailed usage examples, see the `examples` directory. These examples demonstrate common operations such as reading and writing files, and managing directories.

## Contributing

Contributions are welcome! Please follow these steps to contribute:

1. **Fork the Repository:** Create a fork of this repository on GitHub.
2. **Create a Branch:** Create a new branch for your feature or fix.
3. **Make Changes:** Implement your changes in the new branch.
4. **Test Your Changes:** Ensure that your changes do not break existing functionality.
5. **Submit a Pull Request:** Open a pull request with a clear description of your changes.

Please make sure to adhere to the coding style and guidelines described in the `CONTRIBUTING.md` file.

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Contact

For any questions or support, please open an issue on GitHub.

## Disclaimer

This code was made ENTIRELY by ChatGPT, and I do not take any credit whatsoever for it failing or working incredibly well. If you find issues, please still report them, and I will make ChatGPT fix it. Even this README (except this disclaimer of course) was made by ChatGPT
