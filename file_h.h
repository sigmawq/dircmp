//
// Created by swql on 1/5/21.
//

#ifndef BOOST_CRC_TEST_FILE_H_H
#define BOOST_CRC_TEST_FILE_H_H

std::ifstream safe_fopen(std::string& file_path){
    std::ifstream file { file_path };
    if (!file.good()) throw std::runtime_error("Failed to open file");
    return file;
}

// Create new binary file with write permission
static std::fstream new_file_b(std::string& path){
    std::fstream file;
    file.open(path, std::ios::trunc | std::ios::binary | std::ios::out);
    if (!file.good()) throw std::runtime_error("Failed to create file");
    return file;
}

// Create new text file with write permission
static std::fstream new_file_t(std::string& path){
    std::fstream file;
    file.open(path, std::ios::trunc | std::ios::out);
    if (!file.good()) throw std::runtime_error("Failed to create file");
    return file;
}

// Open file as text for reading
static std::ifstream openf_t(std::string& path){
    std::ifstream file;
    file.open(path, std::ios::in);
    if (!file.good()) throw std::runtime_error("Failed to open file");
    return file;
}

// Open file as binary for reading
static std::ifstream openf_b(std::string& path){
    std::ifstream file;
    file.open(path, std::ios::binary);
    if (!file.good()) throw std::runtime_error("Failed to open file");
    return file;
}

// Write a sequence of bytes
// Always end with a null
template<typename char_type>
static void write_nt(std::fstream& fh, char_type* str, size_t size){
    fh.write(str, size);
    fh << 0;
}

#endif //BOOST_CRC_TEST_FILE_H_H
