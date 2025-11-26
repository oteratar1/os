#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lib.h"

int main() {
    
    printf("  1 a b e - интеграл sin(x) от a до b с шагом e\n");
    printf("  2 a b - НОД чисел a и b\n");
    printf("  exit - выход\n");
    
    char input[256];
    
    while (1) {
        printf("\n> ");
        fflush(stdout);
        
        if (!fgets(input, sizeof(input), stdin)) {
            break;
        }
        
        // Убираем символ новой строки
        input[strcspn(input, "\n")] = '\0';
        
        if (strcmp(input, "exit") == 0) {
            break;
        } else if (input[0] == '1') {
            // Интеграл sin(x)
            float a, b, e;
            if (sscanf(input + 1, "%f %f %f", &a, &b, &e) == 3) {
                float result = sin_integral(a, b, e);
                printf("Интеграл sin(x) на [%.2f, %.2f] с шагом %.4f = %.6f\n", 
                       a, b, e, result);
            } else {
                printf("Ошибка: нужно 3 аргумента: a b e\n");
            }
        } else if (input[0] == '2') {
            // НОД
            int a, b;
            if (sscanf(input + 1, "%d %d", &a, &b) == 2) {
                int result = gcd(a, b);
                printf("НОД(%d, %d) = %d\n", a, b, result);
            } else {
                printf("Ошибка: нужно 2 аргумента: a b\n");
            }
        } else {
            printf("Неизвестная команда\n");
        }
    }
    
    return 0;
}