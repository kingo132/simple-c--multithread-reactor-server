#include <iostream>
#include <thread>
#include <vector>
#include <random>
#include <cstring>
#include <mutex>
#include <map>
#include <chrono>
#include <arpa/inet.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>

const int NUM_CLIENTS = 20;
const char* SERVER_IP = "127.0.0.1";
const int SERVER_PORT = 12345;

std::mutex printMutex;

void logMessage(const std::string& message) {
    std::lock_guard<std::mutex> guard(printMutex);
    std::cout << message << std::endl;
}

int generateRandomLength(int minLen, int maxLen) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(minLen, maxLen);
    return dis(gen);
}

std::string generateRandomString(int length) {
    std::string chars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    std::string result;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, chars.size() - 1);

    for (int i = 0; i < length; ++i) {
        result += chars[dis(gen)];
    }
    return result;
}

void clientThreadFunction(int clientId) {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        logMessage("[Client " + std::to_string(clientId) + "] Failed to create socket");
        return;
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0) {
        logMessage("[Client " + std::to_string(clientId) + "] Invalid server IP address");
        close(sockfd);
        return;
    }

    if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        logMessage("[Client " + std::to_string(clientId) + "] Connection failed");
        close(sockfd);
        return;
    }

    logMessage("[Client " + std::to_string(clientId) + "] Connected to server");

    int sequenceNumber = 0;
    std::map<int, std::string> sentMessages;

    while (true) {
        int dataLength = generateRandomLength(8, 8196);
        std::string dataContent = generateRandomString(dataLength - 8);

        int totalLength = dataLength;
        sequenceNumber++;

        // Prepare the message
        char* buffer = new char[dataLength];
        int netLength = htonl(totalLength);
        int netSequence = htonl(sequenceNumber);
        memcpy(buffer, &netLength, 4);
        memcpy(buffer + 4, &netSequence, 4);
        memcpy(buffer + 8, dataContent.c_str(), dataLength - 8);

        // Send the message
        if (send(sockfd, buffer, dataLength, 0) < 0) {
            logMessage("[Client " + std::to_string(clientId) + "] Failed to send data");
            delete[] buffer;
            break;
        }

        sentMessages[sequenceNumber] = dataContent;

        // Receive the response
        char* recvBuffer = new char[dataLength];
        int received = recv(sockfd, recvBuffer, dataLength, 0);
        if (received < 0) {
            logMessage("[Client " + std::to_string(clientId) + "] Failed to receive data");
            delete[] buffer;
            delete[] recvBuffer;
            break;
        }

        // Verify the response
        int recvLength;
        int recvSequence;
        memcpy(&recvLength, recvBuffer, 4);
        memcpy(&recvSequence, recvBuffer + 4, 4);
        recvLength = ntohl(recvLength);
        recvSequence = ntohl(recvSequence);

        if (recvLength != totalLength || recvSequence != sequenceNumber) {
            logMessage("[Client " + std::to_string(clientId) + "] Data verification failed: Length or Sequence Mismatch");
        } else {
            std::string recvContent(recvBuffer + 8, dataLength - 8);
            if (recvContent != sentMessages[recvSequence]) {
                logMessage("[Client " + std::to_string(clientId) + "] Data verification failed: Content Mismatch");
            } else {
                logMessage("[Client " + std::to_string(clientId) + "] Data verification succeeded");
            }
        }

        delete[] buffer;
        delete[] recvBuffer;

        // Simulate some delay between messages
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    close(sockfd);
    logMessage("[Client " + std::to_string(clientId) + "] Disconnected from server");
}

int main() {
    std::vector<std::thread> clientThreads;

    for (int i = 0; i < NUM_CLIENTS; ++i) {
        clientThreads.emplace_back(clientThreadFunction, i);
    }

    for (auto& thread : clientThreads) {
        if (thread.joinable()) {
            thread.join();
        }
    }

    return 0;
}
