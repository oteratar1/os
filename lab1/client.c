#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>


int main() {
    int pipe1[2], pipe2[2];
    pid_t pid;
    char filename[100];
    char user_input[1024];
    char response_buffer[256];
    
    if (pipe(pipe1) == -1 || pipe(pipe2) == -1) {
        perror("Ошибка создания pipe");
        exit(EXIT_FAILURE);
    }
    
    printf("Введите имя файла для результатов: ");
    if (fgets(filename, sizeof(filename), stdin) == NULL) {
        perror("Ошибка чтения имени файла");
        exit(EXIT_FAILURE);
    }
    filename[strcspn(filename, "\n")] = 0;
    
    pid = fork();
    if (pid == -1) {
        perror("Ошибка fork");
        exit(EXIT_FAILURE);
    }
    
    if (pid == 0) {
        // Дочерний процесс
        close(pipe1[1]);
        close(pipe2[0]);
        dup2(pipe1[0], STDIN_FILENO);
        dup2(pipe2[1], STDOUT_FILENO);
        close(pipe1[0]);
        close(pipe2[1]);
        
        execl("./server", "server", filename, NULL);
        perror("Ошибка execl");
        exit(EXIT_FAILURE);
        
    } else {
        // Родительский процесс
        close(pipe1[0]);
        close(pipe2[1]);
        
        printf("Дочерний процесс создан.\n\n");
        
        while (1) {
            printf("Введите числа: ");
            if (fgets(user_input, sizeof(user_input), stdin) == NULL) {
                break;
            }
            
            if (strncmp(user_input, "exit", 4) == 0) {
                break;
            }
            
            // Отправляем данные
            write(pipe1[1], user_input, strlen(user_input));
            
            // Ждем ответ от сервера
            ssize_t bytes_read = read(pipe2[0], response_buffer, sizeof(response_buffer) - 1);
            if (bytes_read > 0) {
                response_buffer[bytes_read] = '\0';
                
                if (strstr(response_buffer, "DIVISION_BY_ZERO") != NULL) {
                    printf("Обнаружено деление на ноль! Завершаем работу.\n");
                    break;
                } else {
                    printf("Сервер: %s", response_buffer);
                }
            } else if (bytes_read == 0) {
                printf("Сервер закрыл соединение\n");
                break;
            }
        }
        
        close(pipe1[1]);
        close(pipe2[0]);
        wait(NULL);
        printf("Родительский процесс завершен.\n");
    }
    
    return 0;
}