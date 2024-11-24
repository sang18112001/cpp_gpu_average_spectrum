cmake_minimum_required(VERSION 3.10)
project(MyCUDAProgram)

# Chỉ định sử dụng CUDA
find_package(CUDA REQUIRED)

# Thêm các file mã nguồn
set(SOURCES main.cpp
        "processor /tuning.cpp"
        platforms/tuning_nvidia.cu)

# Tạo mục tiêu thực thi từ các file mã nguồn
cuda_add_executable(my_program ${SOURCES})
# Liên kết các thư viện cần thiết (nếu cần)
target_link_libraries(my_program ${CUDA_LIBRARIES})
