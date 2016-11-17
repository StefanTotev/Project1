#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include <math.h>

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

//Shared variables that are needed for synchronisation of the threads
int iterationCounter=0,
    elementsBelowPrecision = 0;

double *matrix;


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

/* Iterates over the inner square of numbers in the matrix and
 * adds them to either the first or the second queue.
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
        elementsBelowPrecision++;
    }
}

void calculateJobs(void* attr) {
    //Receives the size, precision and number of threads
    struct inputs *fileInputs = (struct inputs*)attr;

    int innerElementsInMatrix = (fileInputs->size-2)*(fileInputs->size-2),
        queueNumber = 1, //Indicates which of the two queues is being used
        elementsAreAbovePrecision = 1; //Indicates when the loop should stop
    struct jobQueue *head; //Serves as a copy of the current head

    //Iterate over the job queue until all of the elements are under the precision
    while(elementsAreAbovePrecision) {

        //Get the head of the linked list depending on which queue is being used
        if (queueNumber == 1) {
            head = getAndForwardFirstHead();
        } else {
            head = getAndForwardSecondHead();
        }

        //Add the next element in the queue unless all of them are calculated
        if(head != NULL) {
            addElement(head->row,
                       head->column,
                       fileInputs->size,
                       fileInputs->precision);
        }
        //If all the elements in the queue are calculated, wait for all threads
        else if(head == NULL){
            //If all of the elements in the inner matrix have been calculated
            if (queueNumber == 2)  {
                iterationCounter++;
                //Reset the counter for the number of elements that are above
                //below the precision
                if (elementsBelowPrecision < innerElementsInMatrix) {
                    elementsBelowPrecision = 0;
                }
                secondQueueHead = initialSecondQueueHead;
                queueNumber = 1;
            }
            //If the queues have finished executing the first queue, they
            //go to the second one and vice versa
            else if (queueNumber == 1) {
                firstQueueHead = initialFirstQueueHead;
                queueNumber = 2;
            }
            //If all elements in the matrix are below the precision, break the loop
            if (elementsBelowPrecision == innerElementsInMatrix) {
                elementsAreAbovePrecision = 0;
            }
        }
    }
}

int main(int argc, char *argv[])
{
    int size=12, numberOfThreads = 5;
    double precision = 0.002;

    matrix = malloc(size * size * sizeof(double));
    struct inputs *fileInputs = malloc(sizeof(struct inputs));

    fileInputs->size = size;
    fileInputs->numberOfThreads = numberOfThreads;
    fileInputs->precision = precision;

    //Check if memory allocation was successful
    if (matrix == NULL || fileInputs == NULL) {
        printf("Memory allocation failed!");
        return 1;
    }

    populateMatrix(size);
    populateJobQueues(size);

    //Remember the original posit   ion of the heads
    initialFirstQueueHead = firstQueueHead;
    initialSecondQueueHead = secondQueueHead;

    calculateJobs(fileInputs);
    printf("%d", iterationCounter);

    //Free allocated memory and destroy mutexes
    free(matrix);
    return 0;
}
