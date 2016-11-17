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
    struct jobQueue *nextJob;
};
struct jobQueue *firstQueueHead = NULL,
                *secondQueueHead = NULL,
                *initialFirstQueueHead = NULL,
                *initialSecondQueueHead = NULL;


//MOVE VARIABLES TO LOCAL
int waitingThreadsCount = 0,
    matrixRelaxedFlag = 0,
    elementsBelowPrecision = 0,
    size=10,
    iterationCounter=0;

double *mainMatrix,
       *calculatedMatrix,
       precision = 0.002;

pthread_mutex_t lock;
pthread_mutex_t precisionLock;
pthread_mutex_t headLock;
pthread_cond_t waitForThreads;


//Returns head from first queue and moves it to the next element in the queue
struct jobQueue* getAndForwardFirstHead() {

    //Reference to the queue head that will be returned
    struct jobQueue *temporaryJob = firstQueueHead;
    //Move head forward in the queue and return reference previous to head
    if(firstQueueHead == NULL) return NULL;
    else {
        firstQueueHead = firstQueueHead->nextJob;
        return temporaryJob;
    }
}

//Returns head from first queue and moves it to the next element in the queue
struct jobQueue* getAndForwardSecondHead() {

    //Reference to the queue head that will be returned
    struct jobQueue *temporaryJob = secondQueueHead;
    //Move head forward in the queue and return reference previous to head
    if(secondQueueHead == NULL) return NULL;
    else {
        secondQueueHead = secondQueueHead->nextJob;
        return temporaryJob;
    }
}

/* Iterates over the inner square of numbers in the matrix and
 * adds them to either the first or the second queue depending on
 * whether they can be calculated in parallel without the use of locks.
 * Does not need to be locked since it's only used in the sequential
 * part of the code.
 */
void insertElement(int queueNumber, int row, int column) {
    //create a new job
    struct jobQueue *newJob = (struct jobQueue*) malloc(sizeof(struct jobQueue));

    //assign passed arguments to values in the new job
    newJob->row = row;
    newJob->column = column;

    //Depending on the queueNumber, assigns the new job to the appropriate queue
    if (queueNumber == 1){
        newJob->nextJob = firstQueueHead;
        firstQueueHead = newJob;
    } else if (queueNumber == 2) {
        newJob->nextJob = secondQueueHead;
        secondQueueHead = newJob;
    }
}

void populateJobQueues() {

    int i, j;
    //Depending on the position of the element, assign it to one of the queues
    for(i = size - 2; i > 0; i--) {
        for(j = size - 2; j > 0; j--) {
            if((i+j)%2 == 0) {
                insertElement(1, i, j);
            } else if((i+j)%2 != 0) {
                insertElement(2, i, j);
            }
        }
    }
    //Remember the original position of the heads
    initialFirstQueueHead = firstQueueHead;
    initialSecondQueueHead = secondQueueHead;
}

void populateMatrix() {
    //Read from file
    FILE *array = fopen("arrayOfNumbers.txt", "r");

    int i=0;
    float num;
    //Assign as many elements to the matrix as its size
    for(i = 0; i < size * size; i++)
    {
        //Assign element from the file to the two matrices
        if (fscanf(array, "%f", &num) > 0) {
            mainMatrix[i] = num;
            calculatedMatrix[i] = mainMatrix[i];
        } else {
            //If no more elements are left in the file, exit with an error
            printf("Not enough elements in the file!");
            exit(1);
        }
    }
    fclose(array);
}

//Assign the newly calculated elements from the updated to the main matrix
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
    int innerElementsInMatrix = (size-2)*(size-2), queueNumber = 1, elementsAreAbovePrecision = 1;
    struct jobQueue *head;

    while(elementsAreAbovePrecision) {

        pthread_mutex_lock(&headLock);
        if(queueNumber == 1) head = getAndForwardFirstHead();
        else head = getAndForwardSecondHead();
        pthread_mutex_unlock(&headLock);

        if(head != NULL) {
            addElement(head->row, head->column);
        }

        else if(head == NULL){

            pthread_mutex_lock(&lock);

            if(waitingThreadsCount < NUMBER_OF_THREADS-1){
                waitingThreadsCount++;
                pthread_cond_wait(&waitForThreads, &lock);
            }

            else if(waitingThreadsCount == NUMBER_OF_THREADS-1) {
                if (queueNumber == 2)  {
                    iterationCounter++;
                    pthread_mutex_lock(&precisionLock);
                    if (elementsBelowPrecision < innerElementsInMatrix) elementsBelowPrecision = 0;
                    pthread_mutex_unlock(&precisionLock);
                    updateMatrix();
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

            pthread_mutex_unlock(&lock);

            pthread_mutex_lock(&precisionLock);
            if (elementsBelowPrecision == innerElementsInMatrix) elementsAreAbovePrecision = 0;
            pthread_mutex_unlock(&precisionLock);
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
    pthread_mutex_init(&headLock, NULL);

    if (mainMatrix == NULL || calculatedMatrix == NULL) {
        printf("Memory allocation failed!");
        exit(1);
    }

    populateMatrix();
    populateJobQueues();

    relaxMatrix();

    //free memory from jobQueue
    pthread_mutex_destroy(&headLock);
    pthread_mutex_destroy(&precisionLock);
    pthread_mutex_destroy(&lock);
    free(mainMatrix);
    free(calculatedMatrix);
    return 0;
}
