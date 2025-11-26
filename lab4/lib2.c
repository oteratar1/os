#include "lib.h"
#include <math.h>

// Реализация №2: Метод трапеций
float sin_integral(float a, float b, float e) {
    float integral = 0.0;
    float n = (b - a) / e;
    for (int i = 0; i < n; i++) {
        float x1 = a + i * e;
        float x2 = a + (i + 1) * e;
        integral += (sin(x1) + sin(x2)) * e / 2;
    }
    return integral;
}

/* Наивный алгоритм: пытаться разделить числа на 
все числа, что меньше a и b.  */
int gcd(int a, int b) {
    int min = (a < b) ? a : b;
    for (int i = min; i >= 1; i--) {
        if (a % i == 0 && b % i == 0)
            return i;
    }
    return 1;
}