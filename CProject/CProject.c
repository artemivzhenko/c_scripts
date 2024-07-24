#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <windows.h>
#include <time.h>

#pragma comment(lib, "ws2_32.lib")

#define SERVER_ID "SRV"             
#define LOG_FILE "server.log"
#define RECV_LOG_FILE "server_get.log"
#define PORT 8080
#define BUFFER_SIZE 1024

DWORD WINAPI send_thread_func(LPVOID arg);
DWORD WINAPI recv_thread_func(LPVOID arg);
void generate_message(char* buffer);

int main() {
    WSADATA wsa;
    HANDLE send_thread, recv_thread;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        printf("Failed to initialize Winsock. Error Code: %d\n", WSAGetLastError());
        return 1;
    }
    send_thread = CreateThread(NULL, 0, send_thread_func, NULL, 0, NULL);
    if (send_thread == NULL) {
        printf("Failed to create send thread. Error Code: %d\n", GetLastError());
        return 1;
    }
    recv_thread = CreateThread(NULL, 0, recv_thread_func, NULL, 0, NULL);
    if (recv_thread == NULL) {
        printf("Failed to create recv thread. Error Code: %d\n", GetLastError());
        return 1;
    }
    WaitForSingleObject(send_thread, INFINITE);
    WaitForSingleObject(recv_thread, INFINITE);

    WSACleanup();
    return 0;
}
DWORD WINAPI send_thread_func(LPVOID arg) {
    SOCKET sockfd;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd == INVALID_SOCKET) {
        printf("Socket creation failed. Error Code: %d\n", WSAGetLastError());
        return 1;
    }
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    FILE* log_file = fopen(LOG_FILE, "a");
    if (!log_file) {
        printf("Failed to open log file\n");
        closesocket(sockfd);
        return 1;
    }

    while (1) {
        generate_message(buffer);
        sendto(sockfd, buffer, strlen(buffer), 0, (struct sockaddr*)&server_addr, sizeof(server_addr));
        fprintf(log_file, "%s\n", buffer);
        fflush(log_file);

        Sleep(1000);
    }

    fclose(log_file);
    closesocket(sockfd);
    return 0;
}
DWORD WINAPI recv_thread_func(LPVOID arg) {
    SOCKET sockfd;
    struct sockaddr_in server_addr, client_addr;
    char buffer[BUFFER_SIZE];
    int len = sizeof(client_addr);
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd == INVALID_SOCKET) {
        printf("Socket creation failed. Error Code: %d\n", WSAGetLastError());
        return 1;
    }
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;
    if (bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        printf("Bind failed. Error Code: %d\n", WSAGetLastError());
        closesocket(sockfd);
        return 1;
    }
    FILE* recv_log_file = fopen(RECV_LOG_FILE, "a");
    if (!recv_log_file) {
        printf("Failed to open receive log file\n");
        closesocket(sockfd);
        return 1;
    }

    while (1) {
        int n = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr*)&client_addr, &len);
        buffer[n] = '\0';
        char date_time[30];
        sscanf(buffer + 9, "%s", date_time);
        time_t now = time(NULL);
        struct tm tm;
        strptime(date_time, "%Y-%m-%d %H:%M:%S", &tm);
        time_t msg_time = mktime(&tm);
        double elapsed_time = difftime(now, msg_time);
        fprintf(recv_log_file, "Received: %s, Elapsed time: %.2f seconds\n", buffer, elapsed_time);
        fflush(recv_log_file);
    }

    fclose(recv_log_file);
    closesocket(sockfd);
    return 0;
}
void generate_message(char* buffer) {
    static int message_counter = 0;
    char hex_counter[6];
    time_t now = time(NULL);
    struct tm* tm_info = localtime(&now);
    snprintf(hex_counter, 6, "%05X", message_counter++);

    strftime(buffer, BUFFER_SIZE, "%Y-%m-%d %H:%M:%S", tm_info);
    snprintf(buffer, BUFFER_SIZE, "%s%s|%s", SERVER_ID, hex_counter, buffer);
}
