#include "tuning_nvidia.cuh"
#include <iostream>
#include <cuda_runtime.h>

__global__ void gpuProcessKernel(const float* src, float* dst, float val, int len, Operation op) {
    int idx = threadIdx.x + blockIdx.x * blockDim.x;
    if (idx < len) {
        switch (op) {
            case COPY:
                dst[idx] = src[idx];
                break;
            case ZERO:
                dst[idx] = 0.0f;
                break;
            case ADD:
                dst[idx] += src[idx];
                break;
            case MULC:
                dst[idx] *= val;
                break;
        }
    }
}

void gpuProcess_32f(const float* src, float* dst, float val, int len, Operation op) {
    float* d_src = nullptr;
    float* d_dst = nullptr;

    // Cấp phát bộ nhớ trên GPU
    cudaError_t err = cudaMalloc(&d_src, len * sizeof(float));
    if (err != cudaSuccess) {
        std::cerr << "CUDA malloc d_src failed: " << cudaGetErrorString(err) << std::endl;
        return;
    }

    err = cudaMalloc(&d_dst, len * sizeof(float));
    if (err != cudaSuccess) {
        std::cerr << "CUDA malloc d_dst failed: " << cudaGetErrorString(err) << std::endl;
        cudaFree(d_src);
        return;
    }

    // Copy dữ liệu từ CPU -> GPU
    if (src != nullptr) {
        err = cudaMemcpy(d_src, src, len * sizeof(float), cudaMemcpyHostToDevice);
        if (err != cudaSuccess) {
            std::cerr << "CUDA memcpy HostToDevice failed: " << cudaGetErrorString(err) << std::endl;
            cudaFree(d_src);
            cudaFree(d_dst);
            return;
        }
    }

    // Gọi kernel để thực hiện phép toán
    int threadsPerBlock = 256;
    int blocksPerGrid = (len + threadsPerBlock - 1) / threadsPerBlock;
    gpuProcessKernel<<<blocksPerGrid, threadsPerBlock>>>(d_src, d_dst , val, len, op);
    err = cudaGetLastError(); // Kiểm tra lỗi khi chạy kernel
    if (err != cudaSuccess) {
        std::cerr << "CUDA kernel launch failed: " << cudaGetErrorString(err) << std::endl;
        cudaFree(d_src);
        cudaFree(d_dst);
        return;
    }

    // Copy dữ liệu từ GPU -> CPU
    err = cudaMemcpy(dst, d_dst, len * sizeof(float), cudaMemcpyDeviceToHost);
    if (err != cudaSuccess) {
        std::cerr << "CUDA memcpy DeviceToHost failed: " << cudaGetErrorString(err) << std::endl;
    }

    // Giải phóng bộ nhớ
    cudaFree(d_src);
    cudaFree(d_dst);
}
