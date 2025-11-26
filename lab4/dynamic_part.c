#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>      
#include <stdbool.h>
#include "lib.h"

// Объявления типов функций
typedef float (*sin_integral_func)(float, float, float);
typedef int (*gcd_func)(int, int);

int main() {
    void *lib_handle = NULL;
    sin_integral_func sin_integral = NULL;
    gcd_func gcd = NULL;
    
    // Загружаем первую реализацию по умолчанию
    lib_handle = dlopen("./libmy1.so", RTLD_LAZY);
    if (!lib_handle) {
        fprintf(stderr, "Ошибка загрузки библиотеки: %s\n", dlerror());
        return 1;
    }
    
    // Получаем указатели на функции
    sin_integral = (sin_integral_func)dlsym(lib_handle, "sin_integral");
    gcd = (gcd_func)dlsym(lib_handle, "gcd");
    
    if (!sin_integral || !gcd) {
        fprintf(stderr, "Ошибка получения функций: %s\n", dlerror());
        dlclose(lib_handle);
        return 1;
    }

    printf("  0 - переключить реализацию (1 или 2)\n");
    printf("  1 a b e - интеграл sin(x) от a до b с шагом e\n");
    printf("  2 a b - НОД чисел a и b\n");
    printf("  exit - выход\n");
    
    char input[256];
    bool using_lib1 = true;  // true = lib1, false = lib2
    
    while (1) {
        printf("\n> ");
        fflush(stdout);
        
        if (!fgets(input, sizeof(input), stdin)) {
            break;
        }
        
        input[strcspn(input, "\n")] = '\0';
        
        if (strcmp(input, "exit") == 0) {
            break;
        } else if (strcmp(input, "0") == 0) {
            // Переключение реализации
            dlclose(lib_handle);
            
            if (using_lib1) {
                // Переключаемся на lib2
                lib_handle = dlopen("./libmy2.so", RTLD_LAZY);
                printf("Переключено на реализацию 2 (трапеции + наивный алгоритм)\n");
            } else {
                // Переключаемся на lib1
                lib_handle = dlopen("./libmy1.so", RTLD_LAZY);
                printf("Переключено на реализацию 1 (прямоугольники + Евклид)\n");
            }
            
            if (!lib_handle) {
                fprintf(stderr, "Ошибка загрузки библиотеки: %s\n", dlerror());
                return 1;
            }
            
            // Обновляем указатели на функции
            sin_integral = (sin_integral_func)dlsym(lib_handle, "sin_integral");
            gcd = (gcd_func)dlsym(lib_handle, "gcd");
            
            if (!sin_integral || !gcd) {
                fprintf(stderr, "Ошибка получения функций: %s\n", dlerror());
                dlclose(lib_handle);
                return 1;
            }
            
            using_lib1 = !using_lib1;
            
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
    
    dlclose(lib_handle);
    return 0;
}