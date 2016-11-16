#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include <math.h>
#include <pthread.h>

#define NUMBER_OF_THREADS 10

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

pthread_mutex_t lock;
pthread_mutex_t precisionLock;
pthread_cond_t waitForThreads;

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


//Does not need to be locked since it's only used sequentially
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

    FILE *array = fopen("arrayOfNumbers.txt", "r");

    int i=0;
    float num;

    while(fscanf(array, "%f", &num) > 0 && i < size*size) {
        mainMatrix[i] = num;
        calculatedMatrix[i] = mainMatrix[i];
        i++;
    }

    fclose(array);
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
        pthread_mutex_lock(&precisionLock);
        elementsBelowPrecision++;
        pthread_mutex_unlock(&precisionLock);
    }
}

void *calculateJobs(void *attr) {
    int i, j, innerElementsInMatrix = (size-2)*(size-2), queueNumber = 1, id = (int) attr, elementsAreAbovePrecision = 1;
    struct jobQueue *head;
    while(elementsAreAbovePrecision) {
        pthread_mutex_lock(&lock);
        if(queueNumber == 1) head = getAndForwardFirstHead();
        else head = getAndForwardSecondHead();
        pthread_mutex_unlock(&lock);

        if(head != NULL) {
            addElement(head->row, head->column);
        } else if(head == NULL){
            pthread_mutex_lock(&lock);

            if(waitingThreadsCount < NUMBER_OF_THREADS-1){
                waitingThreadsCount++;
                pthread_cond_wait(&waitForThreads, &lock);
            } else if(waitingThreadsCount == NUMBER_OF_THREADS-1) {
                if (queueNumber == 2)  {
                    iterationCounter++;
                    pthread_mutex_lock(&precisionLock);
                    if (elementsBelowPrecision < innerElementsInMatrix) elementsBelowPrecision = 0;
                    pthread_mutex_unlock(&precisionLock);
                    updateMatrix();
                    printMatrix();
                }
                pthread_cond_broadcast(&waitForThreads);
                waitingThreadsCount = 0;
            }

            if (queueNumber == 1) {
                firstQueueHead = initialFirstQueueHead;
                queueNumber = 2;
            } else {
                secondQueueHead = initialSecondQueueHead;
                queueNumber = 1;
            }

            pthread_mutex_lock(&precisionLock);
            if (elementsBelowPrecision == innerElementsInMatrix) elementsAreAbovePrecision = 0;
            pthread_mutex_unlock(&precisionLock);
            pthread_mutex_unlock(&lock);
        }
    }
    pthread_exit((void*) 0);
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
        createReturnCode = pthread_create(&thread[threadId], &attributes, calculateJobs, (void *) threadId);
        if (createReturnCode) {
            printf("ERROR: return code from pthread_create is %d\n", createReturnCode);
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
    calculatedMatrix = malloc(size * size * sizeof(double));

    pthread_mutex_init(&lock, NULL);
    pthread_mutex_init(&precisionLock, NULL);

    if (mainMatrix == NULL || calculatedMatrix == NULL) {
        printf("Memory allocation failed!");
        exit(1);
    }

    populateMatrix();
    populateJobQueues();
    printMatrix();

    relaxMatrix();

    //free memory from jobQueue
    pthread_mutex_destroy(&precisionLock);
    pthread_mutex_destroy(&lock);
    free(mainMatrix);
    free(calculatedMatrix);
    return 0;
}
