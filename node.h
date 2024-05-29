#ifndef NODE_H
#define NODE_H

#include <vector>
#include <string>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <mutex>
#include <thread>
#include "utils.h"

#pragma comment(lib, "Ws2_32.lib")

class Node {
public:
    Node();
    ~Node();
    void startServer();
    void connectToNode(const std::string& ip, int port);
    void sendFileToNode(const std::string& filePath, int port);
    void listenForConnections();
    std::vector<std::pair<std::string, int>> getConnectedNodes() const;
    std::string getIPAddress() const;
    int getPort() const;

private:
    void handleClient(SOCKET clientSocket);
    void processReceivedData(SOCKET clientSocket);
    void sendFile(SOCKET socket, const std::string& filePath);
    std::string getLocalIPAddress() const;
    std::vector<std::pair<std::string, int>> connectedNodeAddresses;
    std::vector<SOCKET> connectedSockets;
    SOCKET listenSocket;
    sockaddr_in serverAddr;
    mutable std::mutex nodeMutex; // Mutex for thread safety
};

#endif
