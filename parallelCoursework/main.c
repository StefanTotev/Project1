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

int waitingThreadsCount = 0, matrixRelaxedFlag = 0, elementsBelowPrecision = 0, size=7;
double *mainMatrix, *updatedMatrix, precision = 0.002;

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

    link->row = row;
    link->column = column;
    link->syncFlag = flag;

    //point it to old first node
    link->nextJob = head;

    //point first to new first node
    head = link;
}

void printList() {
   struct jobQueue *ptr = head;
   printf("\n[ ");

   //start from the beginning
   while(ptr != NULL) {
      printf("(%d,%d,%d) ",ptr->row,ptr->column,ptr->syncFlag);
      ptr = ptr->nextJob;
   }

   printf(" ]");
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

void calculateJobs() {
    int i, j, row, column, elementsInMatrix = (size-2)*(size-2);
    struct jobQueue *tempHead = NULL;

    while(elementsBelowPrecision < elementsInMatrix) {
        tempHead = NULL;

        if(head == NULL) {
            head = initialHead;
            printf("\n%d\n",elementsBelowPrecision);
            elementsBelowPrecision = 0;
            for(i = 0; i < size; i++) {
                for(j = 0; j < size; j++) {
                    mainMatrix[size * i + j] = updatedMatrix[size * i + j];
                }
            }
            printMatrix();
        } else if(head != NULL) {
            tempHead = getAndForwardHead();
        }

        if(tempHead != NULL){

            row = tempHead->row;
            column = tempHead->column;

            updatedMatrix[size*row+column] = (updatedMatrix[size*row + (column - 1)] +
                                              updatedMatrix[size*row + (column + 1)] +
                                              updatedMatrix[size*(row - 1) + column] +
                                              updatedMatrix[size*(row + 1) + column]) / 4;
            if (fabs(mainMatrix[size * row + column] - updatedMatrix[size * row + column]) < precision) {
                elementsBelowPrecision++;
            }
        }
    }
    for(i = 0; i < size; i++) {
                for(j = 0; j < size; j++) {
                    mainMatrix[size * i + j] = updatedMatrix[size * i + j];
                }
            }
    printf("\n%d\n",elementsBelowPrecision);
    printMatrix();

}

int main(int argc, char *argv[])
{
    mainMatrix = malloc(size * size * sizeof(double));
    updatedMatrix = malloc(size * size * sizeof(double));

    if (mainMatrix == NULL || updatedMatrix == NULL) {
        printf("Memory allocation failed!");
        return 1;
    }

    populateMatrix();
    populateJobQueue();
    printList();
    printMatrix();

    calculateJobs();

    //free memory from jobQueue
    free(mainMatrix);
    free(updatedMatrix);
    return 0;
}
