#ifdef __cplusplus
extern "C++"
{
#include "glife.h"
}
#include <cuda.h>

// HINT: YOU CAN USE THIS METHOD FOR ERROR CHECKING
// Print error message on CUDA API or kernel launch
#define cudaCheckErrors(msg)                                   \
    do                                                         \
    {                                                          \
        cudaError_t __err = cudaGetLastError();                \
        if (__err != cudaSuccess)                              \
        {                                                      \
            fprintf(stderr, "Fatal error: %s (%s at %s:%d)\n", \
                    msg, cudaGetErrorString(__err),            \
                    __FILE__, __LINE__);                       \
            fprintf(stderr, "*** FAILED - ABORTING\n");        \
        }                                                      \
    } while (0);

__device__ const int liveTable[2][10] = {{0, 0, 0, 1, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 1, 1, 0, 0, 0, 0, 0}};
// TODO: YOU MAY NEED TO USE IT OR CREATE MORE
__device__ int getNeighbors(int *grid, int tot_rows, int tot_cols,
                            int rows, int cols)
{
    int numOfNeighbors = 0;
    int row = rows - 1 < 0 ? 0 : rows - 1;
    int rows_max = rows + 1 == tot_rows ? rows : rows + 1;
    int cols_max = cols + 1 == tot_cols ? cols : cols + 1;

    for (; row <= rows_max; row++)
    {
        int col = cols - 1 < 0 ? 0 : cols - 1;
        for (; col <= cols_max; col++)
            numOfNeighbors += grid[row * tot_cols + col];
    }
    return numOfNeighbors;
}

// TODO: YOU NEED TO IMPLEMENT KERNEL TO RUN ON GPU DEVICE
__global__ void kernel(int *grid, int *temp, int rows, int cols)
{
    int index = blockDim.x * blockIdx.x + threadIdx.x;
    int num;
    if (index < rows * cols)
    {
        num = getNeighbors(grid, rows, cols, index / cols, index % cols);
        //temp[index] = liveTable[grid[index]][num];
        
        if (num == 3 || (grid[index] && num == 2))
            temp[index] = 1;
        else
            temp[index] = 0;
    }
}
__global__ void swap(int *grid, int *temp, int rows, int cols)
{
    int index = blockDim.x * blockIdx.x + threadIdx.x;
    if (index < rows * cols)
    {
        grid[index] = temp[index];
        temp[index] = 0;
    }
}

// TODO: YOU NEED TO IMPLEMENT TO PRINT THE INDEX RESULTS
void cuda_dump()
{
    printf("===============================\n");

    printf("===============================\n");
}

// TODO: YOU NEED TO IMPLEMENT TO PRINT THE INDEX RESULTS
void cuda_dump_index()
{
    printf(":: Dump Row Column indices\n");
}

// TODO: YOU NEED TO IMPLEMENT ON CUDA VERSION
uint64_t runCUDA(int rows, int cols, int gen,
                 GameOfLifeGrid *g_GameOfLifeGrid, int display)
{
    cudaSetDevice(0); // DO NOT CHANGE THIS LINE

    uint64_t difft;

    // Start timer for CUDA kernel execution
    difft = dtime_usec(0);
    // ---------- TODO: CALL CUDA API HERE ----------


    int size = sizeof(int) * (g_GameOfLifeGrid->getRows() * g_GameOfLifeGrid->getCols());
    int *d_Grid = NULL;
    cudaMalloc((void **)&d_Grid, size);
    int *d_Temp = NULL;
    cudaMalloc((void **)&d_Temp, size);

    cudaMemcpy(d_Grid, *(g_GameOfLifeGrid->getGrid()), size, cudaMemcpyHostToDevice);

    int threadsPerBlock = 256;
    int blocksPerGrid = (size + threadsPerBlock - 1) / threadsPerBlock;

    while (gen--)
    {
        kernel<<<blocksPerGrid, threadsPerBlock>>>(d_Grid, d_Temp, rows, cols);
        swap<<<blocksPerGrid, threadsPerBlock>>>(d_Grid, d_Temp, rows, cols);
    }


    
    cudaMemcpy(*(g_GameOfLifeGrid->getGrid()), d_Grid, size, cudaMemcpyDeviceToHost);

    cudaFree(d_Grid);
    cudaFree(d_Temp);
    // Finish timer for CUDA kernel execution
    difft = dtime_usec(difft);

    // Print the results
    if (display)
    {
        g_GameOfLifeGrid->dump();
        g_GameOfLifeGrid->dumpIndex();
    }
    return difft;
}
#endif
