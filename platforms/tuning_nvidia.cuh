#ifndef MY_KERNELS_H
#define MY_KERNELS_H

#include <cuda_runtime.h>

enum Operation {
    COPY,
    ZERO,
    ADD,
    MULC
};

void gpuProcess_32f(const float* src, float* dst, float val, int len, Operation op);

#endif // MY_KERNELS_H