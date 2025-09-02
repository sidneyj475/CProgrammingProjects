#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <pthread.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include "proj4.h"

/*
 * Structure to pass data to threads for diagonal sum computation.
 * Contains pointers to input and output grids, the target sum,
 * and the range of rows this thread should process.
 */
typedef struct {
    grid *input;          // Input grid containing the digits
    unsigned long s;      // Target sum to match
    grid *output;         // Output grid to mark diagonal sums
    int startRow;         // Starting row for this thread's work
    int endRow;           // Ending row for this thread's work
} ThreadData;

/*
 * Initialize g based on fileName, where fileName
 * is a name of file in the present working directory
 * that contains a valid n-by-n grid of digits, where each
 * digit in the grid is between 1 and 9 (inclusive).
 */
void initializeGrid(grid *g, char *fileName) {
    FILE *file = fopen(fileName, "r");
    if (file == NULL) {
        fprintf(stderr, "Error opening file %s\n", fileName);
        exit(1);
    }

    // Count the number of characters in the first line to determine grid size
    int n = 0;
    int c;
    while ((c = fgetc(file)) != EOF && c != '\n') {
        n++;
    }
    
    // Reset file pointer to beginning
    rewind(file);
    
    // Allocate memory for grid
    g->n = n;
    g->p = (unsigned char **)malloc(n * sizeof(unsigned char *));
    if (g->p == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        fclose(file);
        exit(1);
    }
    
    for (int i = 0; i < n; i++) {
        g->p[i] = (unsigned char *)malloc(n * sizeof(unsigned char));
        if (g->p[i] == NULL) {
            fprintf(stderr, "Memory allocation failed\n");
            // Free previously allocated memory
            for (int j = 0; j < i; j++) {
                free(g->p[j]);
            }
            free(g->p);
            fclose(file);
            exit(1);
        }
    }
    
    // Read grid data
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            c = fgetc(file);
            if (c == '\n') {
                c = fgetc(file);  // Skip newline and read next character
            }
            // Convert char digit to int
            g->p[i][j] = c - '0';
        }
    }
    
    fclose(file);
}

/*
 * Write the contents of g to fileName in the present
 * working directory. If fileName exists in the present working directory, 
 * then this function should overwrite the contents in fileName.
 * If fileName does not exist in the present working directory,
 * then this function should create a new file named fileName
 * and assign read and write permissions to the owner.
 */
void writeGrid(grid *g, char *fileName) {
    int fileDescriptor = open(fileName, O_WRONLY | O_CREAT | O_TRUNC, 0600);
    if (fileDescriptor < 0) {
        fprintf(stderr, "Error opening/creating file %s\n", fileName);
        exit(1);
    }
    
    FILE *file = fdopen(fileDescriptor, "w");
    if (file == NULL) {
        fprintf(stderr, "Error with fdopen for file %s\n", fileName);
        close(fileDescriptor);
        exit(1);
    }
    
    // Write grid data
    for (int i = 0; i < g->n; i++) {
        for (int j = 0; j < g->n; j++) {
            fprintf(file, "%d", g->p[i][j]);
        }
        fprintf(file, "\n");
    }
    
    fclose(file); // This will also close the underlying fileDescriptor
}

/*
 * Free up all dynamically allocated memory used by g.
 * This function should be called when the program is finished using g.
 */
void freeGrid(grid *g) {
    if (g->p == NULL) {
        return;
    }
    
    for (int i = 0; i < g->n; i++) {
        if (g->p[i] != NULL) {
            free(g->p[i]);
            g->p[i] = NULL;
        }
    }
    
    free(g->p);
    g->p = NULL;
    g->n = 0;
}

/*
 * Process all forward diagonals (top-left to bottom-right) starting at (startRow, 0)
 * looking for those with sum equal to s.
 */
void processForwardDiagonalFromLeftEdge(grid *input, unsigned long s, grid *output, int startRow) {
    int n = input->n;
    
    for (int len = 2; len <= n - startRow; len++) {
        unsigned long sum = 0;
        
        // Calculate sum
        for (int k = 0; k < len; k++) {
            sum += input->p[startRow + k][k];
        }
        
        // Mark cells if sum matches target
        if (sum == s) {
            for (int k = 0; k < len; k++) {
                output->p[startRow + k][k] = 1;
            }
        }
    }
}

/*
 * Process all backward diagonals (top-right to bottom-left) starting at (startRow, n-1)
 * looking for those with sum equal to s.
 */
void processBackwardDiagonalFromRightEdge(grid *input, unsigned long s, grid *output, int startRow) {
    int n = input->n;
    
    for (int len = 2; len <= n - startRow; len++) {
        unsigned long sum = 0;
        
        // Calculate sum
        for (int k = 0; k < len; k++) {
            sum += input->p[startRow + k][n - 1 - k];
        }
        
        // Mark cells if sum matches target
        if (sum == s) {
            for (int k = 0; k < len; k++) {
                output->p[startRow + k][n - 1 - k] = 1;
            }
        }
    }
}

/*
 * Process all forward diagonals (top-left to bottom-right) starting at (0, startCol)
 * looking for those with sum equal to s.
 */
void processForwardDiagonalFromTopEdge(grid *input, unsigned long s, grid *output, int startCol) {
    int n = input->n;
    
    for (int len = 2; len <= n - startCol; len++) {
        unsigned long sum = 0;
        
        // Calculate sum
        for (int k = 0; k < len; k++) {
            sum += input->p[k][startCol + k];
        }
        
        // Mark cells if sum matches target
        if (sum == s) {
            for (int k = 0; k < len; k++) {
                output->p[k][startCol + k] = 1;
            }
        }
    }
}

/*
 * Process all backward diagonals (top-right to bottom-left) starting at (0, startCol)
 * looking for those with sum equal to s.
 */
void processBackwardDiagonalFromTopEdge(grid *input, unsigned long s, grid *output, int startCol) {
    // Calculate the maximum diagonal length
    for (int len = 2; len <= startCol + 1; len++) {
        unsigned long sum = 0;
        
        // Calculate sum
        for (int k = 0; k < len; k++) {
            sum += input->p[k][startCol - k];
        }
        
        // Mark cells if sum matches target
        if (sum == s) {
            for (int k = 0; k < len; k++) {
                output->p[k][startCol - k] = 1;
            }
        }
    }
}

/*
 * Thread function to compute diagonal sums for a portion of the grid.
 */
void *diagonalSumsThread(void *arg) {
    ThreadData *data = (ThreadData *)arg;
    grid *input = data->input;
    unsigned long s = data->s;
    grid *output = data->output;
    int startRow = data->startRow;
    int endRow = data->endRow;
    int n = input->n;
    
    // Process all forward and backward diagonals that start from rows in this thread's range
    for (int i = startRow; i < endRow; i++) {
        processForwardDiagonalFromLeftEdge(input, s, output, i);
        processBackwardDiagonalFromRightEdge(input, s, output, i);
    }
    
    // Only first thread processes diagonals starting from the top row
    if (startRow == 0) {
        // Process forward diagonals starting from the top edge (skip column 0, already handled)
        for (int j = 1; j < n; j++) {
            processForwardDiagonalFromTopEdge(input, s, output, j);
        }
        
        // Process backward diagonals starting from the top edge (skip column n-1, already handled)
        for (int j = 0; j < n - 1; j++) {
            processBackwardDiagonalFromTopEdge(input, s, output, j);
        }
    }
    
    return NULL;
}

/*
 * This function will compute all diagonal sums in input that equal s using
 * t threads, where 1 <= t <= 3, and store all of the resulting
 * diagonal sums in output. Each thread should do
 * roughly (doesn't have to be exactly) (100 / t) percent of the 
 * computations involved in calculating the diagonal sums. 
 * This function should call (or call another one of your functions that calls)
 * pthread_create and pthread_join when 2 <= t <= 3 to create additional POSIX
 * thread(s) to compute all diagonal sums.
 */
void diagonalSums(grid *input, unsigned long s, grid *output, int t) {
    int n = input->n;
    
    // Initialize output grid with zeros
    output->n = n;
    output->p = (unsigned char **)malloc(n * sizeof(unsigned char *));
    if (output->p == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(1);
    }
    
    for (int i = 0; i < n; i++) {
        output->p[i] = (unsigned char *)malloc(n * sizeof(unsigned char));
        if (output->p[i] == NULL) {
            fprintf(stderr, "Memory allocation failed\n");
            // Free previously allocated memory
            for (int j = 0; j < i; j++) {
                free(output->p[j]);
            }
            free(output->p);
            exit(1);
        }
        // Initialize with zeros
        for (int j = 0; j < n; j++) {
            output->p[i][j] = 0;
        }
    }
    
    // Single-threaded approach
    if (t == 1) {
        ThreadData data;
        data.input = input;
        data.s = s;
        data.output = output;
        data.startRow = 0;
        data.endRow = n;
        
        diagonalSumsThread(&data);
    } 
    // Multi-threaded approach
    else {
        pthread_t threads[t-1];  // t-1 additional threads (main thread is also used)
        ThreadData threadData[t];
        
        int rowsPerThread = n / t;
        int remainingRows = n % t;
        int currentRow = 0;
        
        // Create and start t-1 additional threads
        for (int i = 0; i < t-1; i++) {
            threadData[i].input = input;
            threadData[i].s = s;
            threadData[i].output = output;
            threadData[i].startRow = currentRow;
            
            // Distribute remaining rows evenly
            int extraRow = (i < remainingRows) ? 1 : 0;
            threadData[i].endRow = currentRow + rowsPerThread + extraRow;
            currentRow = threadData[i].endRow;
            
            if (pthread_create(&threads[i], NULL, diagonalSumsThread, &threadData[i]) != 0) {
                fprintf(stderr, "Error creating thread\n");
                exit(1);
            }
        }
        
        // Main thread handles the last portion
        threadData[t-1].input = input;
        threadData[t-1].s = s;
        threadData[t-1].output = output;
        threadData[t-1].startRow = currentRow;
        threadData[t-1].endRow = n;
        
        diagonalSumsThread(&threadData[t-1]);
        
        // Wait for all threads to complete
        for (int i = 0; i < t-1; i++) {
            if (pthread_join(threads[i], NULL) != 0) {
                fprintf(stderr, "Error joining thread\n");
                exit(1);
            }
        }
    }
}