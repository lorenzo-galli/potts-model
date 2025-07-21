#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <emscripten.h>

EMSCRIPTEN_KEEPALIVE
uint8_t* init_board(int Q, int L) {
    // allocate on the heap so it lives past this function
    int N = L * L;             
    uint8_t *board = malloc(N * sizeof *board);

    // fill with whatever initial pattern you like
    // here for demo with 4 cells:
    board[0] = 0;
    board[1] = 0;
    board[2] = 1;
    board[3] = 0;
    // if N > 4, initialize the rest as needed...

    return board;
}

EMSCRIPTEN_KEEPALIVE
int AddNums(int a, int b) {
    return a + b;
}

int main(){
    printf("Hello, Wolff2D! %d\n", AddNums(6, 4));
}


