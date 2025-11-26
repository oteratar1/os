#include "lib.h"
#include <math.h>

// Реализация №1: Метод прямоугольников
float sin_integral(float a, float b, float e) {
    float integral = 0.0;
    for (float x = a; x < b; x += e) {
        integral += sin(x) * e;
    }
    return integral;
}

/* Алгоритм Евклида */
int gcd(int a, int b) {
    while (b != 0) {
        int t = b;
        b = a % b;
        a = t;
    }
    return a;
}