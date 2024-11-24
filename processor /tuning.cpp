//
// Created by sang on 24/11/2024.
//

#include "tuning.h"

#include <mutex>
#include <thread>

#include "../platforms/tuning_nvidia.cuh"

Tuning::Tuning() {
    buffer_.resize(buffer_size_);

    for (auto& ptr : buffer_) {
        ptr = std::make_unique<float[]>(fftSize_).get();
    }
}

Tuning::~Tuning() {
    buffer_.clear(); // Xóa các con trỏ trong vector
}

void Tuning::aggregateSpectrum(uint64_t ts, int antId, const float *s, int fftSize, uint32_t fs) {
    // Thay thế ippsCopy_32f bằng gpuProcess_32f với phép toán COPY
    gpuProcess_32f(s, buffer_[current_buffer_idx_], 0.0f, fftSize, COPY);
    current_buffer_idx_ = (current_buffer_idx_ + 1) % buffer_.size();

    // std::lock_guard<std::mutex> lock(&specBufferMut_);

    gpuProcess_32f(nullptr, avg_buffer_, 0.0f, fftSize, ZERO);

    for (int j = 0; j < buffer_.size(); j++) {
        gpuProcess_32f(buffer_[j], avg_buffer_, 0.0f, fftSize, ADD);
    }

    float factor = 1.0f / buffer_.size();
    gpuProcess_32f(avg_buffer_, avg_buffer_, factor, fftSize, MULC);

    static uint64_t s_count = 0;
    s_count++;
    if (s_count % 124 == 0) {
        printf("[s_count: %lu] fftSize=%d avg[0]=%f\n", s_count, fftSize, avg_buffer_[0]);
    }
}

void Tuning::printBuffer() const {
    std::cout << "Buffer contents:" << std::endl;
    for (size_t idx = 0; idx < buffer_size_; ++idx) {
        std::cout << "Buffer [" << idx << "]: ";
        for (size_t i = 0; i < fftSize_; ++i) { // Giả định fftSize = 1024
            std::cout << buffer_[idx][i] << " ";
        }
        std::cout << std::endl;
    }
}
