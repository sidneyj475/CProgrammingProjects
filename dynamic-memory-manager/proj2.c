#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

/*
 * Main function to process grades input, dynamically manage memory, compute average, and output results.
 * 
 * Inputs: Reads grades from standard input until a negative value is encountered.
 * Outputs: Prints memory allocation details, average grade, grade comparisons, and total heap usage.
 * Assumptions: Inputs are valid double values; a negative value terminates input.
 *              Uses only permitted functions: printf, scanf, malloc, free.
 */
int main() {
    printf("Enter a list of grades below where each grade is separated by a newline character.\n");
    printf("After the last grade is entered, enter a negative value to end the list.\n");

    size_t currentCapacity = 5;  // Initial capacity (5 grades = 40 bytes)
    size_t gradeCount = 0;       // Number of grades stored
    double *gradesHeap = NULL;   // Pointer to dynamically allocated heap memory
    unsigned int allocCount = 0; // Total heap allocations
    unsigned int freeCount = 0;  // Total heap deallocations
    size_t totalBytes = 0;       // Total bytes allocated
    bool isReading = true;       // Controls input loop

    while (isReading) {
        double grade;
        int scanResult = scanf("%lf", &grade);
        bool isValid = (scanResult == 1 && grade >= 0);

        if (!isValid) {
            isReading = false; // Terminate loop on invalid input or sentinel
        } else {
            // Initial allocation if heap is NULL
            if (gradesHeap == NULL) {
                gradesHeap = malloc(currentCapacity * sizeof(double));
                if (gradesHeap == NULL) {
                    fprintf(stderr, "Memory allocation failed.\n");
                    return 1;
                }
                allocCount++;
                totalBytes += currentCapacity * sizeof(double);
                printf("Allocated %zu bytes to the heap at %p.\n", 
                       currentCapacity * sizeof(double), (void*)gradesHeap);
            }

            // Store the grade and update count
            *(gradesHeap + gradeCount) = grade;
            printf("Stored %f in the heap at %p.\n", grade, (void*)(gradesHeap + gradeCount));
            gradeCount++;

            // Expand heap immediately if full (before reading next input)
            if (gradeCount == currentCapacity) {
                printf("Stored %zu grades (%zu bytes) to the heap at %p.\n", 
                       gradeCount, currentCapacity * sizeof(double), (void*)gradesHeap);
                printf("Heap at %p is full.\n", (void*)gradesHeap);

                size_t newCapacity = currentCapacity + 5; // Increase by 5 grades
                size_t newSize = newCapacity * sizeof(double);
                double *newHeap = malloc(newSize);
                if (newHeap == NULL) {
                    fprintf(stderr, "Memory allocation failed.\n");
                    free(gradesHeap);
                    return 1;
                }

                // Copy existing grades to new heap
                for (size_t i = 0; i < gradeCount; i++) {
                    *(newHeap + i) = *(gradesHeap + i);
                }

                // Log operations and update pointers
                printf("Allocated %zu bytes to the heap at %p.\n", newSize, (void*)newHeap);
                printf("Copied %zu grades from %p to %p.\n", gradeCount, (void*)gradesHeap, (void*)newHeap);
                printf("Freed %zu bytes from the heap at %p.\n", 
                       currentCapacity * sizeof(double), (void*)gradesHeap);
                free(gradesHeap);
                freeCount++;

                gradesHeap = newHeap;
                currentCapacity = newCapacity;
                allocCount++;
                totalBytes += newSize;
            }
        }
    }

    // Compute average
    double sum = 0.0;
    for (size_t i = 0; i < gradeCount; i++) {
        sum += *(gradesHeap + i);
    }
    double average = gradeCount > 0 ? sum / gradeCount : 0.0;
    printf("The average of %zu grades is %f.\n", gradeCount, average);

    // Compare each grade to the average
    for (size_t i = 0; i < gradeCount; i++) {
        double currentGrade = *(gradesHeap + i);
        printf("%zu. The grade of %f is %s the average.\n", 
               i + 1, currentGrade, (currentGrade >= average) ? ">=" : "<");
    }

    // Free remaining heap memory
    if (gradesHeap != NULL) {
        printf("Freed %zu bytes from the heap at %p.\n", 
               currentCapacity * sizeof(double), (void*)gradesHeap);
        free(gradesHeap);
        freeCount++;
    }

    // Print total heap usage
    printf("total heap usage: %u allocs, %u frees, %zu bytes allocated\n", 
           allocCount, freeCount, totalBytes);

    return 0;
}
