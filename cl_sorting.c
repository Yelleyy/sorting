#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CL_TARGET_OPENCL_VERSION 200
#include <CL/cl.h>

#define MAX_SOURCE_SIZE (0x100000)
#define MAX_STRING_LEN 256

// อ่านไฟล์
int readInputFile(const char *filename, char ***data) {
    FILE *file = fopen(filename, "r");
    if (!file) return -1;

    int count = 0, capacity = 100;
    *data = malloc(capacity * sizeof(char *));
    char line[MAX_STRING_LEN];

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

// โหลด OpenCL Kernel จากไฟล์ .cl
cl_program loadProgram(cl_context context, cl_device_id device) {
    FILE *fp = fopen("mysort_opencl.cl", "r");
    if (!fp) { perror("Failed to load"); exit(1); }

    char *source_str = malloc(MAX_SOURCE_SIZE);
    size_t source_size = fread(source_str, 1, MAX_SOURCE_SIZE, fp);
    fclose(fp);

    cl_program program = clCreateProgramWithSource(context, 1, (const char **)&source_str, &source_size, NULL);
    free(source_str);

    clBuildProgram(program, 1, &device, NULL, NULL, NULL);
    return program;
}

// แปลง array ของ string เป็น single buffer เพราะ OpenCL ไม่รองรับ char**
void flattenData(char **data, int count, char *flatData) {
    for (int i = 0; i < count; i++) {
        strncpy(&flatData[i * MAX_STRING_LEN], data[i], MAX_STRING_LEN - 1);
        flatData[i * MAX_STRING_LEN + MAX_STRING_LEN - 1] = '\0';
    }
}

// แปลงกลับเป็น array ของ string
void unflattenData(char **data, int count, char *flatData) {
    for (int i = 0; i < count; i++) {
        strncpy(data[i], &flatData[i * MAX_STRING_LEN], MAX_STRING_LEN);
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <input_file>\n", argv[0]);
        return 1;
    }

    char **data;
    int count = readInputFile(argv[1], &data);
    if (count < 0) {
        perror("Failed to read input file");
        return 1;
    }

    // 1️⃣ เตรียม OpenCL Platform, Context และ Queue
    cl_platform_id platform;
    cl_device_id device;
    cl_context context;
    cl_command_queue queue;

    clGetPlatformIDs(1, &platform, NULL);
    clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, NULL);
    context = clCreateContext(NULL, 1, &device, NULL, NULL, NULL);

    const cl_queue_properties properties[] = {CL_QUEUE_PROPERTIES, CL_QUEUE_PROFILING_ENABLE, 0};
    queue = clCreateCommandQueueWithProperties(context, device, properties, NULL);


    // 2️⃣ โหลด OpenCL Kernel จากไฟล์ .cl
    cl_program program = loadProgram(context, device);
    cl_kernel insertionKernel = clCreateKernel(program, "insertionSortKernel", NULL);
    cl_kernel mergeKernel = clCreateKernel(program, "mergeSortKernel", NULL);

    // 3️⃣ เตรียมข้อมูลส่งไปยัง GPU
    char *flatData = malloc(count * MAX_STRING_LEN);
    flattenData(data, count, flatData);

    cl_mem dataBuffer = clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, 
                                       count * MAX_STRING_LEN, flatData, NULL);

    // 4️⃣ รัน Insertion Sort บน GPU
    size_t globalSize = count;
    int maxStrLen = MAX_STRING_LEN;
    
    clSetKernelArg(insertionKernel, 0, sizeof(cl_mem), &dataBuffer);
    clSetKernelArg(insertionKernel, 1, sizeof(int), &maxStrLen);
    clSetKernelArg(insertionKernel, 2, sizeof(int), &count);

    clEnqueueNDRangeKernel(queue, insertionKernel, 1, NULL, &globalSize, NULL, 0, NULL, NULL);
    clFinish(queue);

    // 5️⃣ รัน Merge Sort บน GPU
    for (int runSize = 1; runSize < count; runSize *= 2) {
        size_t globalSize = count / (2 * runSize);
        clSetKernelArg(mergeKernel, 0, sizeof(cl_mem), &dataBuffer);
        clSetKernelArg(mergeKernel, 1, sizeof(int), &count);
        clSetKernelArg(mergeKernel, 2, sizeof(int), &runSize);
        clEnqueueNDRangeKernel(queue, mergeKernel, 1, NULL, &globalSize, NULL, 0, NULL, NULL);
        clFinish(queue);
    }

    // 6️⃣ ดึงข้อมูลกลับจาก GPU
    clEnqueueReadBuffer(queue, dataBuffer, CL_TRUE, 0, count * MAX_STRING_LEN, flatData, 0, NULL, NULL);
    unflattenData(data, count, flatData);

    // 7️⃣ บันทึกผลลัพธ์ลงไฟล์
    FILE *output = fopen("output_sorted.txt", "w");
    for (int i = 0; i < count; i++) {
        fprintf(output, "%s", data[i]);
        free(data[i]);
    }
    fclose(output);

    // 8️⃣ Cleanup
    free(flatData);
    free(data);
    clReleaseMemObject(dataBuffer);
    clReleaseKernel(insertionKernel);
    clReleaseKernel(mergeKernel);
    clReleaseProgram(program);
    clReleaseCommandQueue(queue);
    clReleaseContext(context);

    printf("Sorting complete. Output saved to output_sorted.txt\n");
    return 0;
}
