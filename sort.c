#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <omp.h>
#include <time.h>
// #define RUN ((n < 1000000) ? 32 : (n < 10000000) ? 64 : (n < 50000000)   ? 128 : 256)
#define RUN 64

int binarySearch(char **arr, char *key, int low, int high)
{
    while (low <= high)
    {
        int mid = low + (high - low) / 2;
        if (strcmp(arr[mid], key) == 0)
            return mid + 1;
        else if (strcmp(arr[mid], key) < 0)
            low = mid + 1;
        else
            high = mid - 1;
    }
    return low;
}
void insertionSort(char **arr, int n)
{
    for (int i = 1; i < n; i++)
    {
        char *key = arr[i];
        int insertionPoint = binarySearch(arr, key, 0, i - 1);

        memmove(&arr[insertionPoint + 1], &arr[insertionPoint], (i - insertionPoint) * sizeof(char *));
        arr[insertionPoint] = key;
    }
}
void merge(char **arr, int left, int mid, int right)
{
    int len1 = mid - left + 1;
    int len2 = right - mid;
    char **leftArr = malloc(len1 * sizeof(char *));
    char **rightArr = malloc(len2 * sizeof(char *));

    #pragma omp parallel
    {
        #pragma omp single
        {
            #pragma omp task
            for (int i = 0; i < len1; i++)
            {
                leftArr[i] = arr[left + i];
            }

            #pragma omp task
            for (int i = 0; i < len2; i++)
            {
                rightArr[i] = arr[mid + 1 + i];
            }

            #pragma omp taskwait
        }
    }
    int i = 0, j = 0, k = left;
    while (i < len1 && j < len2)
    {
        if (strcmp(leftArr[i], rightArr[j]) <= 0)
        {
            arr[k++] = leftArr[i++];
        }
        else
        {
            arr[k++] = rightArr[j++];
        }
    }
    while (i < len1)
    {
        arr[k++] = leftArr[i++];
    }
    while (j < len2)
    {
        arr[k++] = rightArr[j++];
    }
    free(leftArr);
    free(rightArr);
}
void timSort(char **arr, int n)
{
    // #pragma omp parallel
    // {
    //     #pragma omp single
    //     {
            for (int i = 0; i < n; i += RUN)
            {
                int end = (i + RUN < n) ? (i + RUN) : n;
                insertionSort(arr + i, end - i);
            }
            for (int size = RUN; size < n; size *= 2)
            {
                for (int left = 0; left < n; left += 2 * size)
                {
                    int mid = left + size - 1;
                    int right = (left + 2 * size - 1 < n) ? (left + 2 * size - 1) : (n - 1);
                    if (mid < right)
                    {
                        // #pragma omp task
                        // {
                            merge(arr, left, mid, right);
                        // }
                    }
                }
                // #pragma omp taskwait
            }
    //     }
    // }
}
int readInputFile(const char *filename, char ***data)
{
    FILE *file = fopen(filename, "r");
    if (!file)
    {
        perror("Error opening input file");
        return -1;
    }
    char line[1024];
    int count = 0;
    int capacity = 100;
    *data = malloc(capacity * sizeof(char *));
    if (!*data)
    {
        perror("Memory allocation failed");
        fclose(file);
        return -1;
    }
    while (fgets(line, sizeof(line), file))
    {
        if (count == capacity)
        {
            capacity *= 2;
            *data = realloc(*data, capacity * sizeof(char *));
        }
        (*data)[count++] = strdup(line);
    }
    fclose(file);
    return count;
}
void writeOutputFile(const char *filename, char **data, int count)
{
    FILE *file = fopen(filename, "w");
    if (!file)
    {
        perror("Error opening output file");
        return;
    }
    printf("Writing to output file: %s\n", filename);
    for (int i = 0; i < count; i++)
    {
        fprintf(file, "%s", data[i]);
        free(data[i]);
    }
    fclose(file);
    printf("File write complete.\n");
}
int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s <input_file>\n", argv[0]);
        return 1;
    }
    char **data;
    int count = readInputFile(argv[1], &data);
    if (count <= 0)
    {
        fprintf(stderr, "No data read from input file\n");
        return 1;
    }
    clock_t start_time = clock();
    printf("Sorting data...\n");
    timSort(data, count);
    clock_t end_time = clock();
    double elapsed_time = (double)(end_time - start_time) / CLOCKS_PER_SEC;
    printf("Sorting completed in %.6f seconds.\n", elapsed_time);
    writeOutputFile("output", data, count);
    free(data);

    printf("Validating output with valsort...\n");
    char command[1024];
    snprintf(command, sizeof(command), "valsort.exe %s", "output");
    int result = system(command);

    if (result == 0)
    {
        printf("Output validation passed.\n");
    }
    else
    {
        printf("Output validation failed.\n");
    }

    printf("Process complete.\n");
    return 0;
}
