#pragma once

#include <iostream>
#include <cstring>
#include <type_traits>


namespace Utils {

class BufferUtils {
    public:
    template <typename... Args>
    static void deserialize(const char* buffer, Args&... args) {
        const char* ptr = buffer;
        (
            [&]() {
                std::memcpy(&args, ptr, sizeof(args));
                ptr += sizeof(args);
            }
            (), ...
        );
    }

    template <typename... Args>
    static void serialize(char* buffer, const Args&... args) {
        char* ptr = buffer;
        (
            [&]() {
                std::memcpy(ptr, &args, sizeof(args));
                ptr += sizeof(args);
            }
            (), ...
        );
    }
};

}