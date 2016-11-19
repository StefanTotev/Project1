#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include <math.h>
#include <pthread.h>

//Contains the coordinates of the elements that can be calculated in parallel
struct jobQueue {
    int row;
    int column;
    struct jobQueue *nextJob;
};

//Defines the inputs that are passed to the program at run time
struct inputs {
    int size;
    int numberOfThreads;
    double precision;
};

//Defines the two queues and the initial positions of their heads
struct jobQueue *firstQueueHead = NULL,
                *secondQueueHead = NULL,
                *initialFirstQueueHead = NULL,
                *initialSecondQueueHead = NULL;

//Mutexes and condition variables
pthread_mutex_t lock;
pthread_mutex_t precisionLock;
pthread_mutex_t headLock;
pthread_cond_t waitForThreads;

//Shared variables that are needed for synchronisation of the threads
int iterationCounter=0,
    waitingThreadsCount = 0,
    elementsBelowPrecision = 0;

double *matrix;


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

    //Assign passed arguments to values in the new job
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

void populateJobQueues(int size) {

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
}

void populateMatrix(int size) {
    //Read from file
    FILE *array = fopen("arrayOfNumbers.txt", "r");

    int i=0;
    float num;
    //Assign as many elements to the matrix as its size
    for(i = 0; i < size * size; i++)
    {
        //Assign element from the file to the matrix
        if (fscanf(array, "%f", &num) > 0) {
            matrix[i] = num;
        } else {
            //If no more elements are left in the file, exit with an error
            printf("Not enough elements in the file!");
            exit(1);
        }
    }
    fclose(array);
}

//Adds the four numbers surrounding the element identified by "row and "column"
void addElement(int row, int column, int size, double precision){

    double tempValue = matrix[size * row + column], difference;
    //Calculate element
    matrix[size * row + column] =
            (matrix[size * row + (column - 1)] +
             matrix[size * row + (column + 1)] +
             matrix[size * (row - 1) + column] +
             matrix[size * (row + 1) + column]) / 4;

    //Calculate the difference between the calculated element and its old value
    difference=fabs(matrix[size*row+column]-tempValue);

    //If that difference is below the precision, increment "elementsBelowPrecision"
    if (difference < precision) {
        //Locked since it's a shared variable
        pthread_mutex_lock(&precisionLock);
        elementsBelowPrecision++;
        pthread_mutex_unlock(&precisionLock);
    }
}

/* Defines  the repeated execution of the created threads.
 * The threads repeatedly iterate over the first queue of jobs that can be
 * executed in parallel, then wait for all of the threads to finish their
 * executions before continuing to the second queue. The process is repeated with
 * the second queue with the difference that at the end of the second queue,
 * the shared variables are updated and the job queues are reset.
 */
void *calculateJobs(void* attr) {
    //Receives the size, precision and number of threads
    struct inputs *fileInputs = (struct inputs*)attr;

    int innerElementsInMatrix = (fileInputs->size-2)*(fileInputs->size-2),
        queueNumber = 1, //Indicates which of the two queues is being used
        elementsAreAbovePrecision = 1; //Indicates when the loop should stop
    struct jobQueue *head; //Serves as a copy of the current head

    //Iterate over the job queue until all of the elements are under the precision
    while(elementsAreAbovePrecision) {

        //Locked since the thread accesses a shared resource (head of queue)
        pthread_mutex_lock(&headLock);
        //Get the head of the linked list depending on which queue is being used
        if (queueNumber == 1) {
            head = getAndForwardFirstHead();
        } else {
            head = getAndForwardSecondHead();
        }
        pthread_mutex_unlock(&headLock);

        //Add the next element in the queue unless all of them are calculated
        if(head != NULL) {
            addElement(head->row,
                       head->column,
                       fileInputs->size,
                       fileInputs->precision);
        }
        //If all the elements in the queue are calculated, wait for all threads
        else if(head == NULL){

            //Lock since it accesses shared resources
            pthread_mutex_lock(&lock);

            //If the number of threads that are waiting is below the total number
            //of threads, the current thread waits for the last thread to finish
            if(waitingThreadsCount < fileInputs->numberOfThreads-1){
                //Increment the shared variable and release the lock
                waitingThreadsCount++;
                pthread_cond_wait(&waitForThreads, &lock);
            }

            //If all of the other threads have finished their execution and the
            //current thread is the last one,
            else if(waitingThreadsCount == fileInputs->numberOfThreads-1) {
                //If all of the elements in the inner matrix have been calculated
                if (queueNumber == 2)  {
                    iterationCounter++;
                    //Reset the counter for the number of elements that are above
                    //below the precision
                    pthread_mutex_lock(&precisionLock);
                    if (elementsBelowPrecision < innerElementsInMatrix) {
                        elementsBelowPrecision = 0;
                    }
                    pthread_mutex_unlock(&precisionLock);
                    firstQueueHead = initialFirstQueueHead;
                    secondQueueHead = initialSecondQueueHead;
                }

                //Release all waiting threads and reset the counter
                pthread_cond_broadcast(&waitForThreads);
                waitingThreadsCount = 0;
            }

            pthread_mutex_unlock(&lock);

            //If all elements in the matrix are below the precision and this is
            //the end of the iteration over the matrix, break the loop
            pthread_mutex_lock(&precisionLock);
            if (elementsBelowPrecision == innerElementsInMatrix &&
                queueNumber == 2) {
                elementsAreAbovePrecision = 0;
            }
            pthread_mutex_unlock(&precisionLock);

            //If the queues have finished executing the first queue, they
            //go to the second one and vice versa
            if (queueNumber == 1) {
                queueNumber = 2;
            } else {
                queueNumber = 1;
            }
        }
    }
    pthread_exit((void*) 0);
    return NULL;
}

//Initialises, creates and joins threads
void relaxMatrix(struct inputs* fileInputs) {

    int returnCode, i;
    void *status;

    //Initialise threads and their attributes
    pthread_t thread[fileInputs->numberOfThreads];
    pthread_attr_t attributes;
    pthread_attr_init(&attributes);
    pthread_attr_setdetachstate(&attributes, PTHREAD_CREATE_JOINABLE);
    pthread_cond_init(&waitForThreads, NULL);

    //Creates threads and passes the size, precision and number of threads to  them
    for(i = 0; i < fileInputs->numberOfThreads; i++) {
        returnCode = pthread_create(&thread[i],
                                    &attributes,
                                    calculateJobs,
                                    (void *) fileInputs);
        if (returnCode) {
            printf("ERROR: return code from pthread_create is %d\n", returnCode);
            exit(1);
        }
    }

    //Joins the newly created threads to the main thread of execution
    for(i = 0; i < fileInputs->numberOfThreads; i++) {
       returnCode = pthread_join(thread[i], &status);
       if (returnCode) {
          printf("ERROR; return code from pthread_join() is %d\n", returnCode);
          exit(1);
        }
    }

    printf("%d ", iterationCounter);

    pthread_cond_destroy(&waitForThreads);
    pthread_attr_destroy(&attributes);
}

int main(int argc, char *argv[]) {

    clock_t start, end;
    start = clock();

    int size=50, numberOfThreads = 4;
    double precision = 0.002;

    //Allocate memory to variables
    matrix = malloc(size * size * sizeof(double));
    struct inputs *fileInputs = malloc(sizeof(struct inputs));

    fileInputs->size = size;
    fileInputs->numberOfThreads = numberOfThreads;
    fileInputs->precision = precision;

    pthread_mutex_init(&lock, NULL);
    pthread_mutex_init(&precisionLock, NULL);
    pthread_mutex_init(&headLock, NULL);

    //Check if memory allocation was successful
    if (matrix == NULL || fileInputs == NULL) {
        printf("Memory allocation failed!");
        exit(1);
    }

    populateMatrix(size);
    populateJobQueues(size);

    //Remember the original posit   ion of the heads
    initialFirstQueueHead = firstQueueHead;
    initialSecondQueueHead = secondQueueHead;

    relaxMatrix(fileInputs);

    //Free allocated memory and destroy mutexes
    pthread_mutex_destroy(&headLock);
    pthread_mutex_destroy(&precisionLock);
    pthread_mutex_destroy(&lock);
    free(initialFirstQueueHead);
    free(initialSecondQueueHead);
    free(fileInputs);
    free(matrix);
    end = clock();
    printf("Execution time: %f!", ((double) (end - start)) / CLOCKS_PER_SEC);
    return 0;
}
