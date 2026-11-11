#include <stdio.h>
#include "MemoryAllocator.h"

void sortArray(int* arr, int size);
void fillUpArr(int* arr, int size);
void printArr(int* arr, int size);

int main(void)
{
    // Minimal test (Real testing is in tests.c)
    int size = 0;

    printf("Enter length of array: ");
    scanf("%d", &size);
    int* arr = (int*)myMalloc(sizeof(int) * size);
    if(!arr)
    {
        printf("Failed to allocate memory dynamically!\n");
        return EXIT_FAILURE;
    }

    // Fill up the array
    printf("Enter %d numbers:\n", size);
    fillUpArr(arr, size);

    sortArray(arr, size);
    printf("Sorted:\n");

    printArr(arr, size);

    myFree(arr);

    return 0;
}

/*
    Function to receive input to an array
    Input: The array to fill up and its length
    Output: No return values
*/

void fillUpArr(int* arr, int size)
{
    int i = 0;

    for(i = 0; i < size; i++)
    {
        scanf("%d", (arr + i));
    }
}

/*
    Function to sort an array
    Input: The array to sort and its length
    Output: No return values
*/

void sortArray(int* arr, int size)
{
    int i = 0, j = 0, temp = 0;

    for(i = 0; i < size - 1; i++)
    {
        for(j = 0; j < size - i - 1; j++)
        {
            if(*(arr +j) > *(arr + j + 1))
            {
                temp = *(arr + j);
                *(arr + j) = *(arr + j + 1);
                *(arr + j + 1) = temp;
            }
        }
    }
}

/*
    Function to print an array
    Input: The array to print and its length
    Output: No return values
*/

void printArr(int* arr, int size)
{
    int i = 0;

    for(i = 0; i < size; i++)
    {
        printf("%d\n", *(arr + i));
    }
}