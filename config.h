#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdbool.h>

#define N_ATOMS_INIT 10
#define duration 5
#define energy_explode 100000
#define min_split_atomic 10
#define min_atomic_number 50
#define max_atomic_number 100

int energy = 50000;
int splitting=1800;