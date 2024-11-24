//
// Created by sang on 24/11/2024.
//

#ifndef TUNING_H
#define TUNING_H

#include <iostream>
#include <algorithm>

class Tuning {
public:
    Tuning();
    ~Tuning();

    void aggregateSpectrum(uint64_t ts, int antId, const float *s, int fftSize, uint32_t fs);
    void printBuffer() const;

private:
    int current_buffer_idx_ = 0;
    int buffer_size_ = 1000;
    int fftSize_ = 124;
    std::vector<float*> buffer_;
    float *avg_buffer_{};
};



#endif //TUNING_H
