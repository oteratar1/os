#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <fcntl.h>

void write_to_file(int file, const char* message) {
    size_t len = strlen(message);
    if (write(file, message, len) == -1) {
        perror("Ошибка записи в файл");
        exit(EXIT_FAILURE);
    }
}

void process_numbers(int file, const char* input) {
    char buffer[4096];
    strncpy(buffer, input, sizeof(buffer) - 1);
    buffer[sizeof(buffer) - 1] = '\0';

    float numbers[100];
    int count = 0;
    
    // Разбираем строку на числа
    char* token = strtok(buffer, " ");
    while (token != NULL && count < 100) {
        numbers[count++] = atof(token);
        token = strtok(NULL, " ");
    }
    
    if (count < 2) {
        write_to_file(file, "Ошибка: нужно минимум 2 числа\n");
        return;
    }
    
    char result_str[256];
    float first = numbers[0];
    
    // Формируем строку для файла
    snprintf(result_str, sizeof(result_str), "Делим %.2f на:", first);
    write_to_file(file, result_str);
    write_to_file(file, "\n");
    
    // Выполняем деление
    for (int i = 1; i < count; i++) {
        if (fabs(numbers[i]) < 1e-6) {  
            snprintf(result_str, sizeof(result_str), "  ОШИБКА: деление на ноль! %.2f / %.2f\n", first, numbers[i]);
            write_to_file(file, result_str);
    
            const char error_msg[] = "DIVISION_BY_ZERO\n";
            write(STDOUT_FILENO, error_msg, strlen(error_msg));
            exit(EXIT_FAILURE);
        }
        
        float result = first / numbers[i];
        snprintf(result_str, sizeof(result_str), "  %.2f / %.2f = %.2f\n", first, numbers[i], result);
        write_to_file(file, result_str);
    }
    write_to_file(file, "---\n");
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        const char error_msg[] = "Использование: server <имя_файла>\n";
        write(STDERR_FILENO, error_msg, strlen(error_msg));
        exit(EXIT_FAILURE);
    }
    
    const char* filename = argv[1];
    
    int file = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (file == -1) {
        perror("Ошибка открытия файла");
        exit(EXIT_FAILURE);
    }
    
    write_to_file(file, "Сервер запущен. Ожидание чисел...\n");
    
    char input_buffer[4096];
    ssize_t bytes_read;
    
    // Читаем из STDIN (который перенаправлен из pipe1)
    while ((bytes_read = read(STDIN_FILENO, input_buffer, sizeof(input_buffer) - 1)) > 0) {
        input_buffer[bytes_read] = '\0';
        
        
        if (input_buffer[bytes_read - 1] == '\n') {
            input_buffer[bytes_read - 1] = '\0';
        }
       
        if (strlen(input_buffer) == 0) continue;
       
        process_numbers(file, input_buffer);
        
        // Отправляем подтверждение обратно (эхо)
        const char response[] = "OK\n";
        write(STDOUT_FILENO, response, strlen(response));
    }
    
    write_to_file(file, "Сервер завершил работу\n");
    close(file);
    return 0;
}