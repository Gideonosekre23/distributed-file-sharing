#include "node.h"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <thread>
#include <algorithm>
#include <shlobj.h> // For SHGetFolderPath

Node::Node() {
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        std::cerr << "WSAStartup failed: " << result << std::endl;
    }
}

Node::~Node() {
    WSACleanup();
}

void Node::startServer() {
    listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenSocket == INVALID_SOCKET) {
        std::cerr << "Error creating socket: " << WSAGetLastError() << std::endl;
        return;
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    serverAddr.sin_port = htons(0);

    if (bind(listenSocket, reinterpret_cast<SOCKADDR*>(&serverAddr), sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Bind failed with error: " << WSAGetLastError() << std::endl;
        closesocket(listenSocket);
        return;
    }

    int addrLen = sizeof(serverAddr);
    if (getsockname(listenSocket, reinterpret_cast<SOCKADDR*>(&serverAddr), &addrLen) == SOCKET_ERROR) {
        std::cerr << "getsockname failed with error: " << WSAGetLastError() << std::endl;
        closesocket(listenSocket);
        return;
    }

    std::cout << "Server started on port: " << getPort() << std::endl;

    if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "Listen failed with error: " << WSAGetLastError() << std::endl;
        closesocket(listenSocket);
        return;
    }

    std::thread(&Node::listenForConnections, this).detach();
}

void Node::connectToNode(const std::string& ip, int port) {
    SOCKET nodeSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (nodeSocket == INVALID_SOCKET) {
        std::cerr << "Error creating socket: " << WSAGetLastError() << std::endl;
        return;
    }

    sockaddr_in nodeAddr;
    nodeAddr.sin_family = AF_INET;
    nodeAddr.sin_port = htons(port);
    inet_pton(AF_INET, ip.c_str(), &nodeAddr.sin_addr);

    if (connect(nodeSocket, reinterpret_cast<SOCKADDR*>(&nodeAddr), sizeof(nodeAddr)) == SOCKET_ERROR) {
        std::cerr << "Failed to connect to node: " << ip << ":" << port << std::endl;
        closesocket(nodeSocket);
        return;
    }

    std::cout << "Connected to node at " << ip << ":" << port << std::endl;

    {
        std::lock_guard<std::mutex> lock(nodeMutex);
        connectedSockets.push_back(nodeSocket);
        connectedNodeAddresses.emplace_back(ip, port);
    }
}

void Node::sendFileToNode(const std::string& filePath, int port) {
    if (!std::filesystem::exists(filePath)) {
        std::cerr << "File does not exist: " << filePath << std::endl;
        return;
    }

    std::string fileName = std::filesystem::path(filePath).filename().string();

    for (auto& node : connectedNodeAddresses) {
        if (node.second == port) {
            SOCKET nodeSocket = INVALID_SOCKET;
            {
                std::lock_guard<std::mutex> lock(nodeMutex);
                auto it = std::find_if(connectedNodeAddresses.begin(), connectedNodeAddresses.end(), 
                    [&](const std::pair<std::string, int>& n) { return n.second == port; });
                if (it != connectedNodeAddresses.end()) {
                    size_t index = std::distance(connectedNodeAddresses.begin(), it);
                    nodeSocket = connectedSockets[index];
                }
            }

            if (nodeSocket != INVALID_SOCKET) {
                std::ifstream file(filePath, std::ios::binary);
                if (!file) {
                    std::cerr << "Error opening file: " << filePath << std::endl;
                    return;
                }

                // Get file size
                file.seekg(0, std::ios::end);
                std::streampos fileSize = file.tellg();
                file.seekg(0, std::ios::beg);

                // Send filename length and filename
                int fileNameLength = fileName.size();
                send(nodeSocket, reinterpret_cast<const char*>(&fileNameLength), sizeof(fileNameLength), 0);
                send(nodeSocket, fileName.c_str(), fileNameLength, 0);

                // Send file size
                send(nodeSocket, reinterpret_cast<const char*>(&fileSize), sizeof(fileSize), 0);

                // Send file data
                char* fileBuffer = new char[static_cast<size_t>(fileSize)];
                file.read(fileBuffer, fileSize);
                int bytesSent = send(nodeSocket, fileBuffer, static_cast<int>(fileSize), 0);
                delete[] fileBuffer;

                if (bytesSent == SOCKET_ERROR) {
                    std::cerr << "Error sending file to node" << std::endl;
                } else {
                    std::cout << "Sent " << bytesSent << " bytes of file '" << fileName << "' to node." << std::endl;
                }

                file.close();
            } else {
                std::cerr << "Node not found for port " << port << std::endl;
            }
        }
    }
}


void Node::listenForConnections() {
    SOCKET clientSocket;
    sockaddr_in clientAddr;
    int addrLen = sizeof(clientAddr);

    while (true) {
        clientSocket = accept(listenSocket, reinterpret_cast<SOCKADDR*>(&clientAddr), &addrLen);
        if (clientSocket == INVALID_SOCKET) {
            std::cerr << "Accept failed with error: " << WSAGetLastError() << std::endl;
            closesocket(listenSocket);
            WSACleanup();
            return;
        }

        {
            std::lock_guard<std::mutex> lock(nodeMutex);
            connectedSockets.push_back(clientSocket);
            connectedNodeAddresses.emplace_back(inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port));
        }

        std::cout << "Connection accepted from " << inet_ntoa(clientAddr.sin_addr) << ":" << ntohs(clientAddr.sin_port) << std::endl;

        std::thread clientThread(&Node::handleClient, this, clientSocket);
        clientThread.detach();
    }
}

void Node::handleClient(SOCKET clientSocket) {
    processReceivedData(clientSocket);

    {
        std::lock_guard<std::mutex> lock(nodeMutex);
        auto it = std::find(connectedSockets.begin(), connectedSockets.end(), clientSocket);
        if (it != connectedSockets.end()) {
            connectedSockets.erase(it);
        }
    }

    closesocket(clientSocket);
}

void Node::processReceivedData(SOCKET clientSocket) {
    char buffer[1024];
    int bytesReceived;

    // Receive filename length
    int fileNameLength;
    bytesReceived = recv(clientSocket, reinterpret_cast<char*>(&fileNameLength), sizeof(fileNameLength), 0);
    if (bytesReceived <= 0) {
        std::cerr << "Error receiving filename length: " << WSAGetLastError() << std::endl;
        return;
    }

    // Receive filename
    std::string fileName(fileNameLength, '\0');
    bytesReceived = recv(clientSocket, &fileName[0], fileNameLength, 0);
    if (bytesReceived <= 0) {
        std::cerr << "Error receiving filename: " << WSAGetLastError() << std::endl;
        return;
    }

    // Receive file size
    std::streampos fileSize;
    bytesReceived = recv(clientSocket, reinterpret_cast<char*>(&fileSize), sizeof(fileSize), 0);
    if (bytesReceived <= 0) {
        std::cerr << "Error receiving file size: " << WSAGetLastError() << std::endl;
        return;
    }

    // Get Downloads path
    std::string downloadsPath = std::getenv("USERPROFILE");
    downloadsPath += "\\Downloads\\";

    std::string filePath = downloadsPath + fileName;
    std::ofstream receivedFile(filePath, std::ios::binary);

    if (!receivedFile) {
        std::cerr << "Error opening file to save received data." << std::endl;
        return;
    }

    // Receive file data
    while (fileSize > 0) {
        int bytesToReceive = (fileSize < sizeof(buffer)) ? static_cast<int>(fileSize) : sizeof(buffer);
        bytesReceived = recv(clientSocket, buffer, bytesToReceive, 0);
        if (bytesReceived <= 0) {
            std::cerr << "Error receiving file data: " << WSAGetLastError() << std::endl;
            return;
        }
        receivedFile.write(buffer, bytesReceived);
        fileSize -= bytesReceived;
    }

    receivedFile.close();

    std::cout << "File '" << fileName << "' received successfully. Saved to " << filePath << std::endl;
}


std::string Node::getIPAddress() const {
    char ipStr[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(serverAddr.sin_addr), ipStr, INET_ADDRSTRLEN);
    return std::string(ipStr);
}

int Node::getPort() const {
    return ntohs(serverAddr.sin_port);
}

std::vector<std::pair<std::string, int>> Node::getConnectedNodes() const {
    std::lock_guard<std::mutex> lock(nodeMutex);
    return connectedNodeAddresses;
}
