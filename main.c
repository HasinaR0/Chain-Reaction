#define _GNU_SOURCE
// include necessary header files
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdbool.h>
# include "config.h"
// defined all constants
#define N_ATOMS_INIT 10
#define duration 5
#define energy_explode 100000
#define min_split_atomic 10
#define min_atomic_number 50
#define max_atomic_number 100

//================================================================
// function Prototypes
void generateAtoms();
void printReport();
void masterFunc();
void atomProcessFunc(void * );
void activatorFunc();
void feederFunc();
void manageNewAtom(void *);
//================================================================
// Declare global variable 
int atomes[N_ATOMS_INIT];
int wasteAtomes[N_ATOMS_INIT];
_Bool finish = false;
sem_t permit, split, start, stop, semWait;
pthread_cond_t cv = PTHREAD_COND_INITIALIZER;
pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
int waste = 0;
int counter = 0;
int demandEnergy = 0;
//int energy = 50000;
int energyGenerateStep[duration], energyDemandStep[duration];
//================================================================
// Function to generate initial atoms
void generateAtoms() {
    for (int i = 0; i < N_ATOMS_INIT; ++i) {
        int random_atom = rand() % (max_atomic_number - min_atomic_number + 1) + min_atomic_number;
        sem_wait(&semWait);
        atomes[i] = random_atom;
        sem_post(&semWait);
    }
}
//================================================================
// Function to print simulation report
void printReport() {
    int index = 0;
    while (!finish) {
        sleep(1);
        counter++;
        // Update energy and demand energy
        energyGenerateStep[index] = energy;
        energyDemandStep[index] = demandEnergy;
        for (int j = 0; j < N_ATOMS_INIT; j++)
            waste += wasteAtomes[j];
    
        if (counter == duration) {
            // Print the final report

            printf("\nTotal energy generated: %d Energy generated in time%d: %d",
                   energy, index + 1, abs(energyGenerateStep[index] - energyGenerateStep[index - 1]));
            printf("\nTotal energy Demanded: %d Energy Demanded in time%d: %d",
                   demandEnergy, index + 1, abs(energyDemandStep[index] - energyGenerateStep[index - 1]));
            printf("\nWaste = %d", waste);
            finish = true;
            printf("\nTime Over ...");
            exit(0);
        }
        if (index == 0) {

            // Print the report for each time step
            printf("\nEnergy generated in time%d: %d", index + 1, energyGenerateStep[index]);
            printf("\nEnergy Demanded in time%d: %d", index + 1, energyDemandStep[index]);
            printf("\nWaste = %d", waste);
        } else {
            printf("\nTotal energy generated: %d Energy generated in time%d: %d",
                   energy, index + 1, abs(energyGenerateStep[index] - energyGenerateStep[index - 1]));
            printf("\nTotal energy Demanded: %d Energy Demanded in time%d: %d",
                   demandEnergy, index + 1, abs(energyDemandStep[index] - energyDemandStep[index - 1]));
            printf("\nWaste = %d", waste);
        }
        index++;
    }
    return;
}
//================================================================
// Master function controlling the simulation
void masterFunc() {
    generateAtoms();

    // Create threads for reporting, activator and feeder
    pthread_t reportThread, activatorThread, feederThread;
    pthread_t atomProcessThread[N_ATOMS_INIT];

    // initializing semaphores
    sem_init(&permit, 0, 0);
    sem_init(&split, 0, 0);
    sem_init(&start, 0, 0);
    sem_init(&stop, 0, 0);
    sem_init(&semWait, 0, 1);

    // Create reporting, activator and feeder threads
    pthread_create(&reportThread, NULL, (void *)printReport, NULL);
    pthread_create(&activatorThread, NULL, (void *)activatorFunc, NULL);
    pthread_create(&feederThread, NULL, (void *)feederFunc, NULL);

    // Main loop for the simulation 

    while (!finish) {
        usleep(100000);
        pthread_mutex_lock(&m);
        if (atomes[0] == 0) {
            pthread_cond_signal(&cv);
            pthread_mutex_unlock(&m);
            sem_wait(&stop);
        } else {
            pthread_mutex_unlock(&m);
        }

        int i = 0;
        for (; i < N_ATOMS_INIT; ++i) {
            if (atomes[i] != 0) {
                int *temp= malloc(sizeof (int));
                *temp=atomes[i];
                pthread_create(&atomProcessThread[i], NULL, (void *)atomProcessFunc, temp);
                atomes[i] = 0;
            } else {
                break;
            }
        }

        for (int j = 0; j < i; ++j) {
            pthread_join(atomProcessThread[j], NULL);
        }
    }

    // wait for all the threads to finish 
    pthread_join(reportThread, NULL);
    pthread_join(activatorThread, NULL);
    pthread_join(feederThread, NULL);
    return;
}
//================================================================

// Function for processing individual atoms
void atomProcessFunc(void *input) {
    int atom =*((int *) input);
    sem_wait(&semWait);
    demandEnergy += 1000;// 1000 for normal execution and times up, 900 for explosion , 1500 for lack of energy
    energy -= 1000;
    if (finish) {
        sem_post(&semWait);
        return;
    }
    if (energy <= 0) {
        finish = true;
        printf("\nLack of sufficient energy ...(%d)", energy);
        exit(0);
    }
    sem_post(&semWait);

    // notify the activator threads
    sem_post(&permit);
    sem_wait(&split);

    int x = atom / 2;
    int y = atom - x;

    sem_wait(&semWait);
    energy += ((x * y) - ((x > y) ? x : y));
    if (energy > energy_explode) {
        finish = true;
        printf("\nGenerated too much energy caused explosion ...(%d)", energy);
        exit(0);
    }
    sem_post(&semWait);

    // Create threads for managing new atoms resulting from the split
    pthread_t child1, child2;
    int *xx= malloc(sizeof(int));
    *xx=x;
    int *yy= malloc(sizeof(int));
    *yy=y;
    pthread_create(&child1, NULL, (void *)manageNewAtom, xx);
    pthread_create(&child2, NULL, (void *)manageNewAtom, yy);

    pthread_join(child1, NULL);
    pthread_join(child2, NULL);
}
//================================================================

// Function to manage new atoms resulting froom atom splitting
void manageNewAtom(void *input) {
    int atom =*((int *) input);
    sem_wait(&semWait);
    if (atom > min_split_atomic)
        atomes[0] = atom;
    else
        wasteAtomes[0] = atom;
    sem_post(&semWait);
}
//================================================================

// Function to send permission for splitting atmos 
void activatorFunc() {
    while (!finish) {
        sem_wait(&permit);
        //do some operations then make a decision
        sem_post(&split);
        // emit the permission
    }
}
//================================================================

// Function to introduce new atoms to the system
void feederFunc() {
    while (!finish) {
        pthread_mutex_lock(&m);
        while (atomes[0] != 0) {
            pthread_cond_wait(&cv, &m);
        }
        pthread_mutex_unlock(&m);

        // generate new atoms and notify the atom process
        generateAtoms();
        printf("\nFeeder generated new atoms ...");
        sem_post(&stop);
    }
}
//================================================================
int main(int argc, char *argv[]) {

    // create the main thread to control the simulation
    pthread_t mainThread;

    pthread_create(&mainThread, NULL, (void *)masterFunc, NULL);

    // wait for the main thread to finish
    pthread_join(mainThread, NULL);

    // return 0 for indicating sucsessful execution
    return 0;
}
