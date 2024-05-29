# Distributed File Sharing System

The Distributed File Sharing System is a network-based application that enables users to share files among multiple nodes connected over a network.

## Features
- Connect to other nodes over the network.
- Upload files to connected nodes.
- Receive files from connected nodes.
- Easy-to-use command-line interface.

## Getting Started
1. Clone the repository: `git clone <repository-url>`
2. Build the project using CMake:
    ```bash
    cd distributed-file-sharing
    cmake .
    make
    ```
3. Run the executable: `./distributed_file_sharing`

## Usage
- When prompted, choose one of the following options:
    - `1`: Connect to another node.
    - `2`: Upload a file to a connected node.
    - `3`: Exit the program.

## Dependencies
- CMake (version >= 3.10)
- C++ compiler supporting C++17 standard
- Windows operating system (for Winsock API)


