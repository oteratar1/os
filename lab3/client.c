//gcc -o server server.c -lrt -pthread
//gcc -o client client.c -lrt -pthread -lm
//./server results.txt
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/fcntl.h>
#include <sys/mman.h>
#include <semaphore.h>

#define SHM_SIZE 4096

void write_to_file(int file, const char* message) {
    size_t len = strlen(message);
    if (write(file, message, len) == -1) {
        const char msg[] = "Ошибка записи в файл\n";
        write(STDERR_FILENO, msg, sizeof(msg));
    }
}

int process_numbers(int file, const char* input, char* response_buf, size_t response_size) {
    char buffer[4096];
    strncpy(buffer, input, sizeof(buffer) - 1);
    buffer[sizeof(buffer) - 1] = '\0';

    float numbers[100];
    int count = 0;
    
    char* token = strtok(buffer, " ");
    while (token != NULL && count < 100) {
        numbers[count++] = atof(token);
        token = strtok(NULL, " ");
    }
    
    if (count < 2) {
        write_to_file(file, "Ошибка: нужно минимум 2 числа\n");
        snprintf(response_buf, response_size, "ERROR: Need at least 2 numbers\n");
        return 0;
    }
    
    char result_str[256];
    float first = numbers[0];
    
    snprintf(result_str, sizeof(result_str), "Делим %.2f на:", first);
    write_to_file(file, result_str);
    write_to_file(file, "\n");
    
    char temp_response[1024] = "";
    
    for (int i = 1; i < count; i++) {
        if (fabs(numbers[i]) < 1e-6) {  
            snprintf(result_str, sizeof(result_str), "  ОШИБКА: деление на ноль! %.2f / %.2f\n", first, numbers[i]);
            write_to_file(file, result_str);
            
            write_to_file(file, "Сервер завершил работу из-за деления на ноль\n");
            
            return 1;
        }
        
        float result = first / numbers[i];
        snprintf(result_str, sizeof(result_str), "  %.2f / %.2f = %.2f\n", first, numbers[i], result);
        write_to_file(file, result_str);
        
        char temp[50];
        snprintf(temp, sizeof(temp), "%.2f ", result);
        strcat(temp_response, temp);
    }
    write_to_file(file, "---\n");
    
    if (strlen(temp_response) > 0) {
        temp_response[strlen(temp_response) - 1] = '\0';
    }
    snprintf(response_buf, response_size, "OK: %s\n", temp_response);
    
    return 0;
}

char* open_shared_memory(char* shm_name, int* shm_fd) {
    *shm_fd = shm_open(shm_name, O_RDWR, 0);
    if (*shm_fd == -1) {
        const char msg[] = "Ошибка: не удалось открыть shared memory\n";
        write(STDERR_FILENO, msg, sizeof(msg));
        exit(EXIT_FAILURE);
    }

    char* shm_buf = mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, *shm_fd, 0);
    if (shm_buf == MAP_FAILED) {
        const char msg[] = "Ошибка: не удалось отобразить shared memory\n";
        write(STDERR_FILENO, msg, sizeof(msg));
        exit(EXIT_FAILURE);
    }
    
    return shm_buf;
}

sem_t* open_semaphore(char* sem_name) {
    sem_t* sem = sem_open(sem_name, O_RDWR);
    if (sem == SEM_FAILED) {
        const char msg[] = "Ошибка: не удалось открыть семафор\n";
        write(STDERR_FILENO, msg, sizeof(msg));
        exit(EXIT_FAILURE);
    }
    return sem;
}

int main(int argc, char* argv[]) {
    if (argc != 6) {
        const char msg[] = "Использование: client <shm_s2c> <shm_c2s> <sem_s2c> <sem_c2s> <файл_результатов>\n";
        write(STDERR_FILENO, msg, sizeof(msg));
        exit(EXIT_FAILURE);
    }
    
    char* shm_s2c_name = argv[1];
    char* shm_c2s_name = argv[2];
    char* sem_s2c_name = argv[3];
    char* sem_c2s_name = argv[4];
    char* filename = argv[5];
    
    int file = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (file == -1) {
        const char msg[] = "Ошибка открытия файла\n";
        write(STDERR_FILENO, msg, sizeof(msg));
        exit(EXIT_FAILURE);
    }
    
    write_to_file(file, "Сервер запущен. Ожидание чисел...\n");
    
    int shm_s2c_fd, shm_c2s_fd;
    char* shm_s2c_buf = open_shared_memory(shm_s2c_name, &shm_s2c_fd);
    char* shm_c2s_buf = open_shared_memory(shm_c2s_name, &shm_c2s_fd);
    
    sem_t* sem_s2c = open_semaphore(sem_s2c_name);
    sem_t* sem_c2s = open_semaphore(sem_c2s_name);
    
    bool running = true;
    while (running) {
        sem_wait(sem_s2c);
        
        uint32_t* length_from_server = (uint32_t*)shm_s2c_buf;
        char* text_from_server = shm_s2c_buf + sizeof(uint32_t);
        
        if (*length_from_server == UINT32_MAX) {
            running = false;
            break;
        } else if (*length_from_server > 0) {
            text_from_server[*length_from_server] = '\0';
            
            char response[SHM_SIZE - sizeof(uint32_t)];
            int status = process_numbers(file, text_from_server, response, sizeof(response));
            
            uint32_t* length_to_server = (uint32_t*)shm_c2s_buf;
            char* text_to_server = shm_c2s_buf + sizeof(uint32_t);
            
            if (status == 1) {
                *length_to_server = UINT32_MAX;
                running = false;
            } else {
                *length_to_server = strlen(response);
                memcpy(text_to_server, response, *length_to_server);
                text_to_server[*length_to_server] = '\0';
            }
            
            sem_post(sem_c2s);
        }
        
        // Нужно сбросить длину, иначе сервер будет читать старые данные
        *length_from_server = 0;
    }
    
    write_to_file(file, "Сервер завершил работу\n");
    
    sem_close(sem_s2c);
    sem_close(sem_c2s);
    
    munmap(shm_s2c_buf, SHM_SIZE);
    munmap(shm_c2s_buf, SHM_SIZE);
    
    close(shm_s2c_fd);
    close(shm_c2s_fd);
    close(file);
    
    return 0;
}