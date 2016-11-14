#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include <math.h>

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

int main(void)
{
    int size=7, iterationCounter=0;
    double *mainMatrix = malloc(size * size * sizeof(double)),
           *updatedMatrix = malloc(size * size * sizeof(double)),
           precision = 0.002;

    if (mainMatrix == NULL || updatedMatrix == NULL) {
        printf("Memory allocation failed!!!");
        return 1;
    }

    populateMatrix(size, mainMatrix, updatedMatrix);
    printMatrix(size, mainMatrix);

    do {
        addElements(size, updatedMatrix);
        printMatrix(size, updatedMatrix);
        iterationCounter++;
    } while (differenceIsAbovePrecision(size, precision, mainMatrix, updatedMatrix));

    printf("%d ", iterationCounter);

    free(mainMatrix);
    free(updatedMatrix);
    return 0;
}
