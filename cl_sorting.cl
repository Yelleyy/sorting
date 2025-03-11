__kernel void insertionSortKernel(__global char *data, int width, int n) {
    int tid = get_global_id(0); // ID ของ Work Item
    
    for (int i = 1; i < n; i++) {
        int j = i;
        char temp[256];
        for (int t = 0; t < width; t++) temp[t] = data[i * width + t]; 
        
        while (j > 0 && strcmp(&data[(j - 1) * width], temp) > 0) {
            for (int t = 0; t < width; t++) {
                data[j * width + t] = data[(j - 1) * width + t];
            }
            j--;
        }

        for (int t = 0; t < width; t++) {
            data[j * width + t] = temp[t];
        }
    }
}

__kernel void mergeSortKernel(__global char *data, int size, int runSize) {
    int gid = get_global_id(0);

    int left = gid * 2 * runSize;
    int mid = left + runSize - 1;
    int right = min(left + 2 * runSize - 1, size - 1);

    if (mid >= right) return;

    char temp[256];
    int i = left, j = mid + 1, k = 0;

    while (i <= mid && j <= right) {
        if (strcmp(&data[i * 256], &data[j * 256]) <= 0) {
            for (int t = 0; t < 256; t++) temp[k * 256 + t] = data[i * 256 + t];
            i++;
        } else {
            for (int t = 0; t < 256; t++) temp[k * 256 + t] = data[j * 256 + t];
            j++;
        }
        k++;
    }

    while (i <= mid) {
        for (int t = 0; t < 256; t++) temp[k * 256 + t] = data[i * 256 + t];
        i++; k++;
    }

    while (j <= right) {
        for (int t = 0; t < 256; t++) temp[k * 256 + t] = data[j * 256 + t];
        j++; k++;
    }

    for (int t = 0; t < (right - left + 1) * 256; t++) {
        data[left * 256 + t] = temp[t];
    }
}
