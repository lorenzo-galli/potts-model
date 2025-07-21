#include <stdio.h>
#include <emscripten.h>

EMSCRIPTEN_KEEPALIVE
int AddNums(int a, int b) {
    return a + b;
}  

EMSCRIPTEN_KEEPALIVE
int main(){

    printf("Hello, Wolff2D! %d\n", AddNums(6, 4));
}