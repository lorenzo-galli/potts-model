#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdint.h>
#include <emscripten/emscripten.h>

// Degrees of connections for each site - number of neighbors
#define DEGREE 4

// Constants for random number generation
#define FNORM   (2.3283064365e-10)
#define FRANDOM (FNORM * RANDOM) // random double

// Uses an additive feedback with XOR shift register
#define RANDOM  ((ira[ip++] = ira[ip1++] + ira[ip2++]) ^ ira[ip3++])


// Random number generator - do not touch
unsigned myrand, ira[256];
unsigned char ip, ip1, ip2, ip3;

// Prepares the board (max 256 colors)
static uint8_t* board = NULL;

struct var *s0;



// Prepares the seed for the random number generator - LCG Park Miller

unsigned rand4init(void) {
  unsigned long long y;
  
  y = (myrand * 16807LL);
  myrand = (y & 0x7fffffff) + (y >> 31);
  if (myrand & 0x80000000)
    myrand = (myrand&0x7fffffff) + 1;
  return myrand;
}


void initRandom(void) {
  unsigned i;
  
  ip=128;
  ip1=ip-24;
  ip2=ip-55;
  ip3=ip-61;
  
  for (i=ip3; i<ip; i++)
    ira[i] = rand4init();
}



// Fundamental struct - name var
struct var {
  int color, inCluster;
  struct var *neigh[DEGREE]; // points to the array of the 4 nearest neighbors
} **coda; // coda is a pointer that points to an array of pointers which point to struct var
// basically coda has the address of the first element of an array of address of struct var
// The elements of the array point to the atoms in the lattice


// Global variables
int L, N, Q, *count;

// Error function - never called
void error(char *string) {
  fprintf(stderr, "ERROR: %s\n", string);
  exit(EXIT_FAILURE);
}


// Defines the neighbors of each site in the lattice
// s is the pointer to the array of struct var
void initNeigh(struct var * s) {
  int ix, iy, i, j;
  
  // Iterates over the lattice
  for (iy = 0; iy < L; iy++) {
    for (ix = 0; ix < L; ix++) {
      // i is the index of the current site in the lattice
      // j is the index of the neighbor site (changes each time)
      i = ix + iy * L;

      // right neighbor
      j = (ix + 1) % L + iy * L;
      s[i].neigh[0] = s + j;

      // up neighbor
      j = ix + ((iy + 1) % L) * L;
      s[i].neigh[1] = s + j;

      // left neighbor
      j = (ix + L - 1) % L + iy * L;
      s[i].neigh[2] = s + j;

      // down neighbor
      j = ix + ((iy + L - 1) % L) * L;
      s[i].neigh[3] = s + j;
    }
  }
}

// Initializes the colors of the sites in the lattice and sets inCluster to 0
// startFlag = 0 means random colors, 1 means all colors are set to 0
void initColors(struct var * s, int startFlag) {
  int i;

  if (startFlag)
    for (i = 0; i < N; i++) {
      s[i].color = 0;
      s[i].inCluster = 0;
    }
  else
    for (i = 0; i < N; i++) {
      s[i].color = (int)(FRANDOM * Q);
      s[i].inCluster = 0;
    }
}


void oneMetropolisStep(struct var * s, double temperature) {
  int i, num, dE, oldColor, newColor;
  struct var * ps;
  
  // puts all the sites (pointers to the sites) in coda
  for (i = 0; i < N; i++) coda[i] = s + i;
  
  num = N;

  while (num) {
    i = (int)(FRANDOM * num);
    ps = coda[i];
    coda[i] = coda[--num];

    // Basically is accessing the color of the site pointed by ps
    oldColor = ps->color;
    // way to choose a new color
    newColor = (oldColor + 1 + (int)(FRANDOM * (Q-1))) % Q;
    dE = 0;
    // Real metropolis step - sums over the neighbors and calculates the energy difference (without -J)
    for (i = 0; i < DEGREE; i++)
      // is 1 if the neighbor has the same color as oldColor, -1 if it has newColor
      dE += (ps->neigh[i]->color == oldColor) - (ps->neigh[i]->color == newColor);
    
    // if lower energy always switch, if higher switch with probability exp(-dE/T)
    if (dE <= 0 || dE < -temperature * log(FRANDOM))
      ps->color = newColor;
  }
}


// ritorna la dimensione del cluster cambiato
int oneWolffStep(struct var * s, double temperature) {
  
  int newColor, i, iRead, iWrite;
  struct var *ps, *pv; // pointers to the current site and its neighbor
  double prob = 1.-exp(-1./temperature);
  
  // picks a random site and a new color to change to
  coda[0] = s + (int)(FRANDOM * N);
  newColor = (coda[0]->color + 1 + (int)(FRANDOM * (Q-1))) % Q;
  coda[0]->inCluster = 1;
  iRead = 0;
  iWrite = 1;

  // explores the nearest neighbors, if they are the same color and are not
  // in the cluster they are added to the cluster with prob = 1 - e^-(1/T)
  while (iRead < iWrite) {
    ps = coda[iRead++]; // moves forward
    for (i = 0; i < DEGREE; i++) {
      pv = ps->neigh[i];
      if (ps->color == pv->color && pv->inCluster == 0) {
	      if (FRANDOM < prob) {
	      coda[iWrite++] = pv;
	      pv->inCluster = 1;
	      }
      }
    }
    // it sets the color of the site to the new color and goes to explore the rest of the points in the cluster
    ps->color = newColor;
  }

  // stops when the points in the cluster are over, sets inCluster=0 and returns the lenght of the cluster
  for (i = 0; i < iWrite; i++) {
    coda[i]->inCluster = 0;
  }
  return iWrite;
}

EMSCRIPTEN_KEEPALIVE
uint8_t* init_board(int Q_, int L_) {
  // 1) store the incoming parameters into your globals
  Q = Q_;
  L = L_;
  N = L * L;


  // Read the random seed from /dev/random
  FILE *devran = fopen("/dev/random","r");
  fread(&myrand, 4, 1, devran);
  fclose(devran);
  initRandom();

  // 2) free any old buffers
  free(s0);
  free(board);
  free(coda);

  // 3) allocate fresh workspace on the WASM heap
  s0    = calloc(N,      sizeof *s0);
  board = malloc(N      * sizeof *board);
  coda  = malloc(N      * sizeof *coda);

  // 4) build the initial configuration
  initRandom();
  initNeigh(s0);
  initColors(s0, /* random start */ 0);

  // 5) copy colors into the raw byte‐buffer
  for (int i = 0; i < N; i++) {
    board[i] = s0[i].color;
  }

  // 6) hand back the pointer (byte‐offset) into the WASM heap
  return board;
}

EMSCRIPTEN_KEEPALIVE
uint8_t* update_board(double T) {
  oneMetropolisStep(s0, T);
  oneWolffStep(s0, T);

  // refresh the byte‐array
  for (int i = 0; i < N; i++)
    board[i] = s0[i].color;

  return board;
}


int main(void) {
  double Tc;
  struct var *s1;

  double *output; // output[0] = energy, output[1] = order parameter


  // Read the random seed from /dev/random
  FILE *devran = fopen("/dev/random","r");
  fread(&myrand, 4, 1, devran);
  fclose(devran);

  N = L * L;


  // Allocate memory for the variables
  s0 = (struct var *)calloc(N, sizeof(struct var));
  s1 = (struct var *)calloc(N, sizeof(struct var));
  count = (int *)calloc(Q, sizeof(int));
  coda = (struct var **)calloc(N, sizeof(struct var *));
  output = malloc(2*sizeof *output);


  // Uses the functions on s0 (hot start) and s1 (cold start)
  initRandom();
  initNeigh(s0);
  initColors(s0, 0);
  initNeigh(s1);
  initColors(s1, 1);
  
  // orderParam_Energy(s0, output);
  // pythonFrames(s0);
  
  return EXIT_SUCCESS;
}


