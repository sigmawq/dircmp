//
// Created by swql on 1/10/21.
//

#ifndef BOOST_CRC_TEST_HASHF_H
#define BOOST_CRC_TEST_HASHF_H

#include <boost/crc.hpp>
#include <cstring>


namespace{
    // Calculate CRC32 of a buffer
    int32_t crc_32(const char* buffer, size_t length){
        boost::crc_32_type result;
        result.process_bytes(buffer, length);
        int32_t hash = result.checksum();
        return hash;
    }

    // Calculate CRC32 of a buffer
    int32_t crc_32(std::string& buffer){
        boost::crc_32_type result;
        result.process_bytes(buffer.c_str(), buffer.size());
        return result.checksum();
    }

    // Calculate CRC32 of a file
    // Large files will be divided in subparts to void catastrophic memory usage.
    // One big file hash will be then formed out of subhashes
    int32_t crc_32_file(const std::filesystem::path& file_path, size_t sample_size_bytes = 2e+7){
        static char* buffer = new char[sample_size_bytes];

        std::ifstream file;
        file.open(file_path);
        if (!file.good()) {
            throw std::runtime_error("Failed to open: " + file_path.string());
        }

        std::vector<int32_t> hash_array;

        size_t fsize = std::filesystem::file_size(file_path);
        size_t partition_count = fsize / sample_size_bytes;

        hash_array.reserve(partition_count + 1);

        for (size_t p = 0; p < partition_count; p++){
            file.read(buffer, sample_size_bytes);
            hash_array.push_back(crc_32(buffer, sample_size_bytes));
        }

        file.read(buffer, fsize - partition_count * sample_size_bytes);
        hash_array.push_back(crc_32(buffer, fsize - partition_count * sample_size_bytes));

        return crc_32((const char*)hash_array.data(), hash_array.size() * sizeof(int32_t));
    }
}

#endif //BOOST_CRC_TEST_HASHF_H
