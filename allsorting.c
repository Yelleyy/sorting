#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <omp.h>

#define RUN 64


// Binary search for insertion sort
int binarySearch(char **arr, char *key, int low, int high) {
    while (low <= high) {
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

// Insertion Sort
void insertionSort(char **arr, int n) {
    for (int i = 1; i < n; i++) {
        char *key = arr[i];
        int loc = binarySearch(arr, key, 0, i - 1);
        memmove(&arr[loc + 1], &arr[loc], (i - loc) * sizeof(char *));
        arr[loc] = key;
    }
}


// Quick Sort
int partition(char **arr, int low, int high) {
    char *pivot = arr[high];
    int i = low - 1;
    for (int j = low; j < high; j++) {
        if (strcmp(arr[j], pivot) < 0) {
            i++;
            char *temp = arr[i];
            arr[i] = arr[j];
            arr[j] = temp;
        }
    }
    char *temp = arr[i + 1];
    arr[i + 1] = arr[high];
    arr[high] = temp;
    return i + 1;
}

void quickSort(char **arr, int low, int high) {
    if (low < high) {
        int pi = partition(arr, low, high);
        quickSort(arr, low, pi - 1);
        quickSort(arr, pi + 1, high);
    }
}

void _merge(char **arr, int left, int mid, int right) {
    int len1 = mid - left + 1;
    int len2 = right - mid;
    char **leftArr = malloc(len1 * sizeof(char *));
    char **rightArr = malloc(len2 * sizeof(char *));

    for (int i = 0; i < len1; i++) leftArr[i] = arr[left + i];
    for (int i = 0; i < len2; i++) rightArr[i] = arr[mid + 1 + i];

    int i = 0, j = 0, k = left;
    while (i < len1 && j < len2)
        arr[k++] = (strcmp(leftArr[i], rightArr[j]) <= 0) ? leftArr[i++] : rightArr[j++];
    
    while (i < len1) arr[k++] = leftArr[i++];
    while (j < len2) arr[k++] = rightArr[j++];
    
    free(leftArr);
    free(rightArr);
}

// Merge Sort
void mergeSort(char **arr, int left, int right) {
    if (left < right) {
        int mid = left + (right - left) / 2;
        mergeSort(arr, left, mid);
        mergeSort(arr, mid + 1, right);
        _merge(arr, left, mid, right);
    }
}

void mySort(char **arr, int n) {
    #pragma omp parallel for
    for (int i = 0; i < n; i += RUN) {
        int end = (i + RUN < n) ? (i + RUN) : n;
        insertionSort(arr + i, end - i);
    }

    for (int size = RUN; size < n; size *= 2) {
        #pragma omp parallel for
        for (int left = 0; left < n; left += 2 * size) {
            int mid = left + size - 1;
            int right = (left + 2 * size - 1 < n) ? (left + 2 * size - 1) : (n - 1);
            if (mid < right) {
                _merge(arr, left, mid, right);
            }
        }
    }
}

void mergeSortWrapper(char **arr, int n) {
    mergeSort(arr, 0, n - 1);
}

// Wrapper for Quick Sort
void quickSortWrapper(char **arr, int n) {
    quickSort(arr, 0, n - 1);
}

// Standard C Library qsort Wrapper
int compareStrings(const void *a, const void *b) {
    return strcmp(*(const char **)a, *(const char **)b);
}

// File Handling Functions
int readInputFile(const char *filename, char ***data) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Error opening input file");
        return -1;
    }
    char line[1024];
    int count = 0, capacity = 100;
    *data = malloc(capacity * sizeof(char *));
    while (fgets(line, sizeof(line), file)) {
        if (count == capacity) {
            capacity *= 2;
            *data = realloc(*data, capacity * sizeof(char *));
        }
        (*data)[count++] = strdup(line);
    }
    fclose(file);
    return count;
}

void benchmarkSorting(char **data, int count, void (*sortFunction)(char **, int), const char *sortName, const char *outputFilename) {
    char **copy = malloc(count * sizeof(char *));
    for (int i = 0; i < count; i++) copy[i] = strdup(data[i]);

    clock_t start_time = clock();
    sortFunction(copy, count);
    clock_t end_time = clock();

    double elapsed_time = (double)(end_time - start_time) / CLOCKS_PER_SEC;
    printf("%s took %.6f seconds.\n", sortName, elapsed_time);

    FILE *file = fopen(outputFilename, "w");
    if (!file) {
        perror("Error opening output file");
        return;
    }
    for (int i = 0; i < count; i++) {
        fprintf(file, "%s", copy[i]);
        free(copy[i]);
    }
    fclose(file);
    free(copy);

    char command[1024];
    snprintf(command, sizeof(command), "valsort %s", outputFilename);
    printf("Validating %s with valsort...\n", outputFilename);
    system(command);
}

// Main Function
int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <input_file>\n", argv[0]);
        return 1;
    }

    char **data;
    int count = readInputFile(argv[1], &data);
    if (count <= 0) {
        fprintf(stderr, "No data read from input file\n");
        return 1;
    }

    printf("Benchmarking sorting algorithms...\n");
    benchmarkSorting(data, count, mergeSortWrapper, "Merge Sort", "output/output_merge.txt");
    benchmarkSorting(data, count, quickSortWrapper, "Quick Sort", "output/output_quick.txt");
    benchmarkSorting(data, count, mySort, "My Sort", "output/output_mysort.txt");
    benchmarkSorting(data, count, (void (*)(char **, int))qsort, "qsort", "output/output_qsort.txt");

    free(data);
    return 0;
}
