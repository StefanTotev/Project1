#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include <math.h>
#include <pthread.h>

#define NUMBER_OF_THREADS 5

void populateMatrix(int size,
                    double *mainMatrix,
                    double *updatedMatrix) {

    FILE *array = fopen("arrayOfNumbers.txt", "r");

    int i=0;
    float num;

    while(fscanf(array, "%f", &num) > 0 && i < size*size) {
        mainMatrix[i] = num;
        updatedMatrix[i] = mainMatrix[i];
        i++;
    }

    fclose(array);
}

void printMatrix(int size,
                 double *matrix) {
    int i, j;

    for (i = 0; i < size; i++) {
        for(j = 0; j < size; j++) {
            printf("%f ", matrix[size * i + j]);
        }
        printf("\n");
    }
    printf("\n");
}

void addElements(int size,
                 double *matrix) {
    int i, j;

    for(i = 1; i < size; i++) {
        for(j = 1; j < size; j++) {
            if(i>0 && i<(size-1) && j>0 && j<(size-1)) {
                matrix[size*i+j] = (matrix[size*i + (j - 1)] +
                                    matrix[size*i + (j + 1)] +
                                    matrix[size*(i - 1) + j] +
                                    matrix[size*(i + 1) + j]) / 4;
            }
        }
    }
}

bool differenceIsAbovePrecision(int size,
                              double precision,
                              double *originalMatrix,
                              double *updatedMatrix) {
    int i, j;

    for(i = 1; i < size - 1; i++) {
        for(j = 1; j < size - 1; j++) {
            if (fabs(originalMatrix[size * i + j] - updatedMatrix[size * i + j]) > precision ||
                fabs(updatedMatrix[size * i + j] - originalMatrix[size * i + j]) > precision) {
                for(i = 0; i < size; i++) {
                    for(j = 0; j < size; j++) {
                        originalMatrix[size * i + j] = updatedMatrix[size * i + j];
                    }
                }
                return true;
            }
        }
    }

    return false;
}

void *BusyWork(void *t) {
    int i, tid = (int)t;
    double result=0.0;
    printf("Thread %d starting...\n",tid);
    for (i=0; i<1000000; i++) {
        result = result + sin(i) * tan(i);
    }
    printf("Thread %d done. Result = %e\n", tid, result);
    pthread_exit((void*) t);
    return 0;
}

int main(int argc, char *argv[])
{
    int size=10, iterationCounter=0, returnCode, i;
    void *status;
    double *mainMatrix = malloc(size * size * sizeof(double)),
           *updatedMatrix = malloc(size * size * sizeof(double)),
           precision = 0.002;

    if (mainMatrix == NULL || updatedMatrix == NULL) {
        printf("Memory allocation failed!!!");
        return 1;
    }

    pthread_t thread[NUMBER_OF_THREADS];
    pthread_attr_t attributes;
    pthread_attr_init(&attributes);
    pthread_attr_setdetachstate(&attributes, PTHREAD_CREATE_JOINABLE);

    for(i = 0; i < NUMBER_OF_THREADS; i++) {
        printf("Main: creating thread %d\n", i);
        returnCode = pthread_create(&thread[i], &attributes, BusyWork, (void *)i);
        if (returnCode) {
            printf("ERROR: pthread_create() returned code %d\n", returnCode);
            return 1;
        }
    }

    pthread_attr_destroy(&attributes);

    for(i = 0; i < NUMBER_OF_THREADS; i++) {
        returnCode = pthread_join(thread[i], &status);
        if (returnCode) {
            printf("ERROR: pthread_join() returned code %d\n", returnCode);
            return 1;
        }
        printf("Main: completed join with thread %d having a status of %d\n", i, (int)status);
    }


    /*populateMatrix(size, mainMatrix, updatedMatrix);
    printMatrix(size, mainMatrix);

    do {
        addElements(size, updatedMatrix);
        printMatrix(size, updatedMatrix);
        iterationCounter++;
    } while (differenceIsAbovePrecision(size, precision, mainMatrix, updatedMatrix));

    printf("%d ", iterationCounter);*/

    printf("Main: program completed. Exiting.\n");
    pthread_exit(NULL);
    free(mainMatrix);
    free(updatedMatrix);
    return 0;
}
