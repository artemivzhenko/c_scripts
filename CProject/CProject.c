#include <stdio.h>        // Подключение стандартной библиотеки ввода-вывода
#include <stdlib.h>       // Подключение стандартной библиотеки для работы с памятью и другими функциями
#include <string.h>       // Подключение стандартной библиотеки для работы со строками
#include <winsock2.h>     // Подключение библиотеки Winsock для работы с сетевыми соединениями в Windows
#include <windows.h>      // Подключение библиотеки Windows API для работы с потоками и другими функциями
#include <time.h>         // Подключение стандартной библиотеки для работы с временем

#pragma comment(lib, "ws2_32.lib")  // Подключение библиотеки ws2_32.lib для работы с Winsock

#define SERVER_ID "SRV"             // Определение идентификатора сервера
#define LOG_FILE "server.log"       // Определение имени файла для логов отправленных сообщений
#define RECV_LOG_FILE "server_get.log"  // Определение имени файла для логов принятых сообщений
#define PORT 8080                   // Определение порта для UDP соединений
#define BUFFER_SIZE 1024            // Определение размера буфера для сообщений

// Объявление функций потоков и вспомогательных функций
DWORD WINAPI send_thread_func(LPVOID arg);
DWORD WINAPI recv_thread_func(LPVOID arg);
void generate_message(char* buffer);

int main() {
    WSADATA wsa;                     // Структура для хранения информации о версии Winsock
    HANDLE send_thread, recv_thread; // Переменные для хранения дескрипторов потоков

    // Инициализация библиотеки Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        printf("Failed to initialize Winsock. Error Code: %d\n", WSAGetLastError());
        return 1;
    }

    // Создание потока для отправки сообщений
    send_thread = CreateThread(NULL, 0, send_thread_func, NULL, 0, NULL);
    if (send_thread == NULL) {
        printf("Failed to create send thread. Error Code: %d\n", GetLastError());
        return 1;
    }

    // Создание потока для приема сообщений
    recv_thread = CreateThread(NULL, 0, recv_thread_func, NULL, 0, NULL);
    if (recv_thread == NULL) {
        printf("Failed to create recv thread. Error Code: %d\n", GetLastError());
        return 1;
    }

    // Ожидание завершения обоих потоков
    WaitForSingleObject(send_thread, INFINITE);
    WaitForSingleObject(recv_thread, INFINITE);

    WSACleanup();  // Завершение работы с Winsock
    return 0;      // Завершение программы
}

// Функция потока для отправки сообщений
DWORD WINAPI send_thread_func(LPVOID arg) {
    SOCKET sockfd;                  // Переменная для хранения сокета
    struct sockaddr_in server_addr; // Структура для хранения адреса сервера
    char buffer[BUFFER_SIZE];       // Буфер для сообщения

    // Создание UDP сокета
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd == INVALID_SOCKET) {
        printf("Socket creation failed. Error Code: %d\n", WSAGetLastError());
        return 1;
    }

    // Настройка адреса сервера
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    // Открытие лог-файла для записи отправленных сообщений
    FILE* log_file = fopen(LOG_FILE, "a");
    if (!log_file) {
        printf("Failed to open log file\n");
        closesocket(sockfd);
        return 1;
    }

    while (1) {
        generate_message(buffer);  // Генерация сообщения

        // Отправка сообщения по UDP
        sendto(sockfd, buffer, strlen(buffer), 0, (struct sockaddr*)&server_addr, sizeof(server_addr));
        // Запись сообщения в лог-файл
        fprintf(log_file, "%s\n", buffer);
        fflush(log_file);

        Sleep(1000);  // Ожидание 1 секунду перед следующей отправкой
    }

    fclose(log_file);  // Закрытие лог-файла
    closesocket(sockfd);  // Закрытие сокета
    return 0;  // Завершение потока
}

// Функция потока для приема сообщений
DWORD WINAPI recv_thread_func(LPVOID arg) {
    SOCKET sockfd;                  // Переменная для хранения сокета
    struct sockaddr_in server_addr, client_addr;  // Структуры для хранения адресов сервера и клиента
    char buffer[BUFFER_SIZE];       // Буфер для сообщения
    int len = sizeof(client_addr);  // Размер адреса клиента

    // Создание UDP сокета
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd == INVALID_SOCKET) {
        printf("Socket creation failed. Error Code: %d\n", WSAGetLastError());
        return 1;
    }

    // Настройка адреса сервера
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    // Привязка сокета к адресу сервера
    if (bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        printf("Bind failed. Error Code: %d\n", WSAGetLastError());
        closesocket(sockfd);
        return 1;
    }

    // Открытие лог-файла для записи принятых сообщений
    FILE* recv_log_file = fopen(RECV_LOG_FILE, "a");
    if (!recv_log_file) {
        printf("Failed to open receive log file\n");
        closesocket(sockfd);
        return 1;
    }

    while (1) {
        // Прием сообщения по UDP
        int n = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr*)&client_addr, &len);
        buffer[n] = '\0';  // Завершение строки

        char date_time[30];  // Буфер для хранения даты и времени сообщения
        sscanf(buffer + 9, "%s", date_time);  // Извлечение даты и времени из сообщения

        time_t now = time(NULL);  // Текущее время
        struct tm tm;  // Структура для хранения разбитого времени
        strptime(date_time, "%Y-%m-%d %H:%M:%S", &tm);  // Преобразование строки в структуру времени
        time_t msg_time = mktime(&tm);  // Преобразование структуры времени в time_t

        double elapsed_time = difftime(now, msg_time);  // Вычисление прошедшего времени

        // Запись принятого сообщения и времени в лог-файл
        fprintf(recv_log_file, "Received: %s, Elapsed time: %.2f seconds\n", buffer, elapsed_time);
        fflush(recv_log_file);
    }

    fclose(recv_log_file);  // Закрытие лог-файла
    closesocket(sockfd);  // Закрытие сокета
    return 0;  // Завершение потока
}

// Функция для генерации сообщения
void generate_message(char* buffer) {
    static int message_counter = 0;  // Счетчик сообщений
    char hex_counter[6];  // Буфер для шестнадцатеричного счетчика
    time_t now = time(NULL);  // Текущее время
    struct tm* tm_info = localtime(&now);  // Преобразование времени в локальное время

    snprintf(hex_counter, 6, "%05X", message_counter++);  // Преобразование счетчика в шестнадцатеричную строку

    // Формирование строки даты и времени
    strftime(buffer, BUFFER_SIZE, "%Y-%m-%d %H:%M:%S", tm_info);
    // Формирование окончательного сообщения
    snprintf(buffer, BUFFER_SIZE, "%s%s|%s", SERVER_ID, hex_counter, buffer);
}
