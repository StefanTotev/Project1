#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include <math.h>
#include <pthread.h>

#define NUMBER_OF_THREADS 5

struct jobQueue {
    int row;
    int column;
    int syncFlag;
    struct jobQueue *nextJob;
};
struct jobQueue *head = NULL;
struct jobQueue *initialHead = NULL;

int waitingThreadsCount = 0, matrixRelaxedFlag = 0, elementsBelowPrecision = 0, size=10, iterationCounter=0;
double *mainMatrix, *updatedMatrix, precision = 0.002;

pthread_mutex_t lock;
pthread_cond_t waitForThreads;

/*void freeJobQueue() {
    struct jobQueue *curr = head;
    pthread_mutex_lock(&lock);
    while ((curr = head) != NULL) { // set curr to head, stop if list empty.
    head = head->nextJob;          // advance head to next element.
    free (curr);                // delete saved pointer.
}
    pthread_mutex_unlock(&lock);

}*/

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
    struct jobQueue *link = (struct jobQueue*) malloc(sizeof(struct jobQueue));

    pthread_mutex_lock(&lock);
    link->row = row;
    link->column = column;
    link->syncFlag = flag;

    //point it to old first node
    link->nextJob = head;

    //point first to new first node
    head = link;
    pthread_mutex_unlock(&lock);
}

void populateJobQueue() {

    int i, j;

    for(i = 1; i < size - 1; i++) {
        for(j = 1; j < size - 1; j++) {
            if((i+j)%2 != 0) {
                insertElement(i, j, 0);
            }
        }
    }
    for(i = 1; i < size - 1; i++) {
        for(j = 1; j < size - 1; j++) {
            if((i+j)%2 == 0) {
                if(i==1 && j==1) insertElement(i, j, 1);
                else insertElement(i, j, 0);
            }
        }
    }
    initialHead = head;
}

void populateMatrix() {

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

void *calculateJobs() {
    int i, j, row, column, elementsInMatrix = (size-2)*(size-2);
    struct jobQueue *tempHead = NULL;

    while(elementsBelowPrecision < elementsInMatrix) {
        tempHead = NULL;
        pthread_mutex_lock(&lock);

        if(head == NULL && waitingThreadsCount < NUMBER_OF_THREADS-1) {
            waitingThreadsCount++;
            pthread_cond_wait(&waitForThreads, &lock);
        } else if(head == NULL && waitingThreadsCount == NUMBER_OF_THREADS-1) {
            head = initialHead;
            elementsBelowPrecision = 0;
            for(i = 0; i < size; i++) {
                for(j = 0; j < size; j++) {
                    mainMatrix[size * i + j] = updatedMatrix[size * i + j];
                }
            }
            iterationCounter++;
            pthread_cond_broadcast(&waitForThreads);
        } else if(head != NULL) {
            tempHead = getAndForwardHead();
        }

        pthread_mutex_unlock(&lock);

        if(tempHead != NULL){

            row = tempHead->row;
            column = tempHead->column;

            updatedMatrix[size*row+column] = (updatedMatrix[size*row + (column - 1)] +
                                              updatedMatrix[size*row + (column + 1)] +
                                              updatedMatrix[size*(row - 1) + column] +
                                              updatedMatrix[size*(row + 1) + column]) / 4;
            if (fabs(mainMatrix[size * row + column] - updatedMatrix[size * row + column]) < precision) {
                pthread_mutex_lock(&lock);
                elementsBelowPrecision++;
                pthread_mutex_unlock(&lock);
            }
        }
    }

    pthread_exit(NULL);
}

void relaxMatrix() {

    int createReturnCode, joinReturnCode, threadId;
    void *status;

    pthread_t thread[NUMBER_OF_THREADS];
    pthread_attr_t attributes;
    pthread_attr_init(&attributes);
    pthread_attr_setdetachstate(&attributes, PTHREAD_CREATE_JOINABLE);
    pthread_cond_init(&waitForThreads, NULL);
    //Check if status from creating a condition variable is 0 or not

    for(threadId = 0; threadId < NUMBER_OF_THREADS; threadId++) {
        createReturnCode = pthread_create(&thread[threadId], &attributes, calculateJobs, NULL);
        if (createReturnCode) {
            printf("ERROR: pthread_create() returned code %d\n", createReturnCode);
            exit(1);
        }
    }

    for(threadId = 0; threadId < NUMBER_OF_THREADS; threadId++) {
       joinReturnCode = pthread_join(thread[threadId], &status);
       if (joinReturnCode) {
          printf("ERROR; return code from pthread_join() is %d\n", joinReturnCode);
          exit(1);
          }
       }

    printf("%d ", iterationCounter);

    pthread_cond_destroy(&waitForThreads);
    pthread_attr_destroy(&attributes);
}

int main(int argc, char *argv[])
{
    mainMatrix = malloc(size * size * sizeof(double));
    updatedMatrix = malloc(size * size * sizeof(double));

    pthread_mutex_init(&lock, NULL);

    if (mainMatrix == NULL || updatedMatrix == NULL) {
        printf("Memory allocation failed!");
        exit(1);
    }

    populateMatrix();
    populateJobQueue();
    printMatrix();

    relaxMatrix();
    printMatrix();

    //free memory from jobQueue
    pthread_mutex_destroy(&lock);
    free(mainMatrix);
    free(updatedMatrix);
    return 0;
}
