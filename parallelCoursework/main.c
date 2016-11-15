#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include <math.h>

struct jobQueue {
    int row;
    int column;
    int syncFlag;
    struct jobQueue *nextJob;
};
struct jobQueue *head = NULL;
struct jobQueue *initialHead = NULL;

int waitingThreadsCount = 0, matrixRelaxedFlag = 0, elementsBelowPrecision = 0, size=8, iterationCounter=0;
double *mainMatrix, *calculatedMatrix, precision = 0.002;

struct jobQueue* getAndForwardHead() {

    //save reference to first link
    struct jobQueue *temporaryJob = head;

    //mark next to first link as first
    head = head->nextJob;

    //return the deleted link
    return temporaryJob;
}

void insertElement(int row, int column, int flag) {
    //create a link
    struct jobQueue *newJob = (struct jobQueue*) malloc(sizeof(struct jobQueue));

    newJob->row = row;
    newJob->column = column;
    newJob->syncFlag = flag;

    //point it to old first node
    newJob->nextJob = head;

    //point first to new first node
    head = newJob;
}

void populateJobQueue() {

    int i, j;

    for(i = 1; i < size - 1; i++) {
        for(j = 1; j < size - 1; j++) {
            if((i+j)%2 != 0) {
                if(i==1 && j==2) insertElement(i, j, 1);
                else insertElement(i, j, 0);
            }
        }
    }
    for(i = 1; i < size - 1; i++) {
        for(j = 1; j < size - 1; j++) {
            if((i+j)%2 == 0) {
                insertElement(i, j, 0);
            }
        }
    }
    initialHead = head;
}

void populateMatrix() {

    FILE *matrix = fopen("arrayOfNumbers.txt", "r");

    int i=0;
    float number;

    while(fscanf(matrix, "%f", &number) > 0 && i < size*size) {
        mainMatrix[i] = number;
        calculatedMatrix[i] = mainMatrix[i];
        i++;
    }

    fclose(matrix);
}

void updateMatrix() {
    int i, j;
    for(i = 0; i < size; i++) {
        for(j = 0; j < size; j++) {
            mainMatrix[size * i + j] = calculatedMatrix[size * i + j];
        }
    }
}

void printMatrix() {
    int i, j;

    for (i = 0; i < size; i++) {
        for(j = 0; j < size; j++) {
            printf("%f ", mainMatrix[size * i + j]);
        }
        printf("\n");
    }
    printf("\n");
}

void calculateJobs() {
    int row, column, innerElementsInMatrix = (size-2)*(size-2);
    struct jobQueue *tempHead;
    while(elementsBelowPrecision < innerElementsInMatrix) {
        tempHead = getAndForwardHead();

        row = tempHead->row;
        column = tempHead->column;

        calculatedMatrix[size*row+column] = (calculatedMatrix[size*row + (column - 1)] +
                                          calculatedMatrix[size*row + (column + 1)] +
                                          calculatedMatrix[size*(row - 1) + column] +
                                          calculatedMatrix[size*(row + 1) + column]) / 4;
        if (fabs(mainMatrix[size * row + column] - calculatedMatrix[size * row + column]) < precision) {
            elementsBelowPrecision++;
        }

        if (tempHead->syncFlag == 1) {
            head = initialHead;
            if(elementsBelowPrecision < innerElementsInMatrix) elementsBelowPrecision=0;
            updateMatrix();
            iterationCounter++;
            printMatrix();
        }
    }
    free(tempHead);
}

int main(int argc, char *argv[])
{

    mainMatrix = malloc(size * size * sizeof(double));
    calculatedMatrix = malloc(size * size * sizeof(double));

    if (mainMatrix == NULL || calculatedMatrix == NULL) {
        printf("Memory allocation failed!");
        return 1;
    }

    populateMatrix();
    populateJobQueue();
    printMatrix();

    calculateJobs();
    printf("%d", iterationCounter);

    //free memory from jobQueue
    free(mainMatrix);
    free(calculatedMatrix);
    return 0;
}
