#include "node.h"
#include <iostream>
#include <string>
#include <thread>
#include <chrono>

int main() {
    std::cout << "Starting node..." << std::endl;
    Node node;
    node.startServer();

    std::cout << "Node started. IP: " << node.getIPAddress() << ", Port: " << node.getPort() << std::endl;

    // Start listening for connections in a separate thread
    std::thread listenerThread(&Node::listenForConnections, &node);

    while (true) {
        if (node.getConnectedNodes().empty()) {
            std::cout << "No nodes connected. Please connect to a node or wait for a connection." << std::endl;
            std::string input;
            std::cout << "Enter 'connect' to establish a connection or 'wait' to wait for a connection: ";
            std::cin >> input;

            if (input == "connect") {
                std::string destIP;
                int destPort;
                std::cout << "Enter IP of the node to connect to: ";
                std::cin >> destIP;
                std::cout << "Enter port of the node to connect to: ";
                std::cin >> destPort;
                std::cin.ignore();

                node.connectToNode(destIP, destPort);
            } else if (input == "wait") {
                // Just wait for the listener thread to handle incoming connections
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        } else {
            std::cout << "Options:\n1. Connect to another node\n2. Upload a file\n3. Exit\n";
            std::string choice;
            std::cout << "Enter your choice: ";
            std::cin >> choice;

            if (choice == "1") {
                std::string destIP;
                int destPort;
                std::cout << "Enter IP of the node to connect to: ";
                std::cin >> destIP;
                std::cout << "Enter port of the node to connect to: ";
                std::cin >> destPort;
                std::cin.ignore();

                node.connectToNode(destIP, destPort);
            } else if (choice == "2") {
                if (node.getConnectedNodes().empty()) {
                    std::cout << "No connected nodes to upload the file to. Please connect to a node first." << std::endl;
                } else {
                    std::string filePath;
                    int destPort;
                    std::cout << "Enter the file path to upload: ";
                    std::cin >> filePath;
                    std::cout << "Enter the port of the node to send the file to: ";
                    std::cin >> destPort;
                    std::cin.ignore();

                    node.sendFileToNode(filePath, destPort);
                }
            } else if (choice == "3") {
                std::cout << "Exiting the program." << std::endl;
                break;
            } else {
                std::cout << "Invalid input. Please try again." << std::endl;
            }
        }
    }

    // Join the listener thread to ensure it finishes before exiting
    listenerThread.join();

    return 0;
}
