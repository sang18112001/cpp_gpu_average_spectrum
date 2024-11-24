#include <iostream>
#include "platforms/tuning_nvidia.cuh"
#include "processor /tuning.h"

class Source {
public:
    int antenna() const { return 1; } // ID antenna giả lập
    int device_type() const { return 0; } // Loại thiết bị
};

class Spectrum {
public:
    uint64_t timestamp() const { return 123456789; } // Timestamp giả lập
    Source source() const { return Source(); }       // Nguồn dữ liệu
    int sample_count() const { return 1000; }        // 1000 mẫu
    uint32_t bandwidth() const { return 48000; }     // Băng thông

    // Tạo dữ liệu mẫu với 1000 mẫu và mỗi mẫu có 124 giá trị
    std::vector<std::vector<float>> sample() const {
        std::vector<std::vector<float>> samples;

        // Tạo 1000 mẫu, mỗi mẫu có 124 giá trị
        for (int i = 0; i < 1000; ++i) {
            std::vector<float> sample;
            for (int j = 0; j < 124; ++j) {
                // Tạo giá trị ngẫu nhiên cho mỗi phần tử trong mẫu (ví dụ ngẫu nhiên từ 0 đến 10)
                sample.push_back(static_cast<float>(rand()) / static_cast<float>(RAND_MAX) * 10);
            }
            samples.push_back(sample);
        }

        return samples;
    }
};

int main() {
    Tuning tuning;          // Tạo đối tượng Tuning
    Spectrum spt;           // Tạo đối tượng Spectrum (mock)

    for (const auto& sample : spt.sample()) {
        tuning.aggregateSpectrum(
            spt.timestamp(),          // Timestamp
            spt.source().antenna(),   // Antenna ID
            sample.data(),            // Dữ liệu nguồn
            spt.sample_count(),       // Số lượng mẫu
            spt.bandwidth()           // Băng thông
        );
    }
    // tuning.printBuffer();

    return 0;
}
