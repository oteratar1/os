#include <fcntl.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/fcntl.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <sys/mman.h>

#define SHM_SIZE 4096

char* create_shared_memory(char* shm_name, int* shm_fd) {
    shm_unlink(shm_name);
    
    *shm_fd = shm_open(shm_name, O_RDWR | O_CREAT | O_TRUNC, 0600);
    if (*shm_fd == -1) {
        const char msg[] = "Ошибка: не удалось создать shared memory\n";
        write(STDERR_FILENO, msg, sizeof(msg));
        exit(EXIT_FAILURE);
    }

    if (ftruncate(*shm_fd, SHM_SIZE) == -1) {
        const char msg[] = "Ошибка: не удалось установить размер shared memory\n";
        write(STDERR_FILENO, msg, sizeof(msg));
        exit(EXIT_FAILURE);
    }

    char* shm_buf = mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, *shm_fd, 0);
    if (shm_buf == MAP_FAILED) {
        const char msg[] = "Ошибка: не удалось отобразить shared memory\n";
        write(STDERR_FILENO, msg, sizeof(msg));
        exit(EXIT_FAILURE);
    }
    
    uint32_t* length = (uint32_t*)shm_buf;
    *length = 0;

    return shm_buf;
}

sem_t* create_semaphore(char* sem_name) {
    sem_unlink(sem_name);
    sem_t* sem = sem_open(sem_name, O_RDWR | O_CREAT | O_TRUNC, 0600, 0);
    if (sem == SEM_FAILED) {
        const char msg[] = "Ошибка: не удалось создать семафор\n";
        write(STDERR_FILENO, msg, sizeof(msg));
        exit(EXIT_FAILURE);
    }
    return sem;
}

int main(int argc, char** argv) {
    if (argc != 2) {
        const char msg[] = "Использование: server <имя_файла>\n";
        write(STDERR_FILENO, msg, strlen(msg));
        exit(EXIT_FAILURE);
    }

    char* filename = argv[1];
    
    char shm_s2c_name[64];
    char shm_c2s_name[64];
    char sem_s2c_name[64];
    char sem_c2s_name[64];

    snprintf(shm_s2c_name, sizeof(shm_s2c_name), "lab3-shm-s2c-%d", getpid());
    snprintf(shm_c2s_name, sizeof(shm_c2s_name), "lab3-shm-c2s-%d", getpid());
    snprintf(sem_s2c_name, sizeof(sem_s2c_name), "lab3-sem-s2c-%d", getpid());
    snprintf(sem_c2s_name, sizeof(sem_c2s_name), "lab3-sem-c2s-%d", getpid());

    int shm_s2c_fd, shm_c2s_fd;
    char* shm_s2c_buf = create_shared_memory(shm_s2c_name, &shm_s2c_fd);
    char* shm_c2s_buf = create_shared_memory(shm_c2s_name, &shm_c2s_fd);
    
    sem_t* sem_s2c = create_semaphore(sem_s2c_name);
    sem_t* sem_c2s = create_semaphore(sem_c2s_name);

    pid_t client_pid = fork();
    
    if (client_pid == 0) {
        char* args[] = {
            "./client",
            shm_s2c_name,
            shm_c2s_name,
            sem_s2c_name,
            sem_c2s_name,
            filename,
            NULL
        };
        
        execv("./client", args);
        
        const char msg[] = "Ошибка: не удалось запустить клиент\n";
        write(STDERR_FILENO, msg, sizeof(msg));
        exit(EXIT_FAILURE);
        
    } else if (client_pid == -1) {
        const char msg[] = "Ошибка: не удалось создать процесс клиента\n";
        write(STDERR_FILENO, msg, sizeof(msg));
        exit(EXIT_FAILURE);
    }

    printf("Сервер запущен. Дочерний процесс PID: %d\n", client_pid);
    printf("Вводите числа (первое делится на остальные), 'exit' для выхода:\n");
    
    bool running = true;
    while (running) {
        char input_buf[SHM_SIZE - sizeof(uint32_t)];
        printf("Введите числа: ");
        fflush(stdout);
        
        if (fgets(input_buf, sizeof(input_buf), stdin) == NULL) {
            break;
        }
        
        size_t input_len = strlen(input_buf);
        if (input_len > 0 && input_buf[input_len - 1] == '\n') {
            input_buf[input_len - 1] = '\0';
            input_len--;
        }
        
        if (strcmp(input_buf, "exit") == 0) {
            printf("Завершение работы...\n");
               uint32_t* length_to_client = (uint32_t*)shm_s2c_buf;
                *length_to_client = UINT32_MAX;  //  сигнал завершения
                 sem_post(sem_s2c);               // разбудить клиента

                running = false;
                break;
                }
        
        if (input_len <= 0) {
            continue;
        }
        
        uint32_t* length_to_client = (uint32_t*)shm_s2c_buf;
        char* text_to_client = shm_s2c_buf + sizeof(uint32_t);
        
        *length_to_client = input_len;
        memcpy(text_to_client, input_buf, input_len);
        text_to_client[input_len] = '\0';
        
        sem_post(sem_s2c);
        
        sem_wait(sem_c2s);
        
        uint32_t* length_from_client = (uint32_t*)shm_c2s_buf;
        char* text_from_client = shm_c2s_buf + sizeof(uint32_t);
        
        if (*length_from_client == UINT32_MAX) {
            printf("Обнаружено деление на ноль! Завершаем работу.\n");
            running = false;
        } else if (*length_from_client > 0) {
            text_from_client[*length_from_client] = '\0';
            printf("Сервер: %s", text_from_client);
        }
        
        *length_from_client = 0;
    }
    
    if (running) {
        uint32_t* length_to_client = (uint32_t*)shm_s2c_buf;
        *length_to_client = UINT32_MAX;
        sem_post(sem_s2c);
    }
    
    waitpid(client_pid, NULL, 0);
    
    sem_close(sem_s2c);
    sem_close(sem_c2s);
    sem_unlink(sem_s2c_name);
    sem_unlink(sem_c2s_name);
    
    munmap(shm_s2c_buf, SHM_SIZE);
    munmap(shm_c2s_buf, SHM_SIZE);
    
    shm_unlink(shm_s2c_name);
    shm_unlink(shm_c2s_name);
    
    close(shm_s2c_fd);
    close(shm_c2s_fd);
    
    printf("Сервер завершил работу.\n");
    
    return 0;
}