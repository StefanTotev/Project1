#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include <math.h>

//Create a structure containing the elements of the matrix that can be computed in parallel
struct jobQueue {
    int row;
    int column;
    struct jobQueue *nextJob;
};
struct jobQueue *firstQueueHead = NULL;
struct jobQueue *secondQueueHead = NULL;
struct jobQueue *initialFirstQueueHead = NULL;
struct jobQueue *initialSecondQueueHead = NULL;

int waitingThreadsCount = 0, matrixRelaxedFlag = 0, elementsBelowPrecision = 0, size=6, iterationCounter=0;
double *mainMatrix, *calculatedMatrix, precision = 0.002;

struct jobQueue* getAndForwardFirstHead() {

    //save reference to first link
    struct jobQueue *temporaryJob = firstQueueHead;
    //mark next to first link as first
    if(firstQueueHead == NULL) return NULL;
    else {
        firstQueueHead = firstQueueHead->nextJob;
        return temporaryJob;
    }
}

struct jobQueue* getAndForwardSecondHead() {

    //save reference to first link
    struct jobQueue *temporaryJob = secondQueueHead;
    //mark next to first link as first
    if(secondQueueHead == NULL) return NULL;
    else {
        secondQueueHead = secondQueueHead->nextJob;
        return temporaryJob;
    }
}

void printList(struct jobQueue* head) {
   struct jobQueue *ptr = head;
   printf("\n[ ");

   //start from the beginning
   while(ptr != NULL) {
      printf("(%d,%d) ",ptr->row,ptr->column);
      ptr = ptr->nextJob;
   }

   printf(" ]");
}

void insertElement(int queueNumber, int row, int column) {
    //create a link
    struct jobQueue *link = (struct jobQueue*) malloc(sizeof(struct jobQueue));

    link->row = row;
    link->column = column;

    //point it to old first node
    if (queueNumber == 1){
        link->nextJob = firstQueueHead;
        firstQueueHead = link;
    } else if (queueNumber == 2) {
        link->nextJob = secondQueueHead;
        secondQueueHead = link;
    }
}

void populateJobQueues() {

    int i, j;

    for(i = size - 2; i > 0; i--) {
        for(j = size - 2; j > 0; j--) {
            if((i+j)%2 == 0) {
                insertElement(1, i, j);
            } else if((i+j)%2 != 0) {
                insertElement(2, i, j);
            }
        }
    }
    initialFirstQueueHead = firstQueueHead;
    initialSecondQueueHead = secondQueueHead;
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

void addElement(int row, int column){

    calculatedMatrix[size*row+column] = (calculatedMatrix[size*row + (column - 1)] +
                                      calculatedMatrix[size*row + (column + 1)] +
                                      calculatedMatrix[size*(row - 1) + column] +
                                      calculatedMatrix[size*(row + 1) + column]) / 4;
    if (fabs(mainMatrix[size * row + column] - calculatedMatrix[size * row + column]) < precision) {
        elementsBelowPrecision++;
    }
}

void calculateJobs() {
    int innerElementsInMatrix = (size-2)*(size-2), queueNumber = 1, elementsAreAbovePrecision = 1, i, j;
    struct jobQueue *head;
    while(elementsAreAbovePrecision) {

        if(queueNumber == 1) head = getAndForwardFirstHead();
        else head = getAndForwardSecondHead();

        if(head != NULL) {
            addElement(head->row, head->column);
        } else if(head == NULL){
            if (queueNumber == 1) {
                firstQueueHead = initialFirstQueueHead;
                queueNumber = 2;
            } else {
                secondQueueHead = initialSecondQueueHead;
                queueNumber = 1;
                iterationCounter++;
                if (elementsBelowPrecision < innerElementsInMatrix) elementsBelowPrecision = 0;
                updateMatrix();
                printMatrix();
            }
            if (elementsBelowPrecision == innerElementsInMatrix) elementsAreAbovePrecision = 0;
        }
    }
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
    populateJobQueues();
    printMatrix();

    calculateJobs();
    printf("%d", iterationCounter);

    //free memory from jobQueue
    free(mainMatrix);
    free(calculatedMatrix);
    return 0;
}
