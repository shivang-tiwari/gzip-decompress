
#include <iostream>
#include <vector>

namespace Utils {

class StreamUtils {
    public:

    static void read_bytes_into_vector(std::istream& stream, std::vector<uint8_t> &res) {
        size_t n = res.size();
        stream.read(reinterpret_cast<char*>(res.data()), static_cast<std::streamsize>(n));
        std::streamsize bytes_read = stream.gcount();
        if (bytes_read >= 0 && static_cast<size_t>(bytes_read) < n) {
            res.resize(static_cast<size_t>(bytes_read));
        }
    }

    static std::vector<uint8_t> read_n_bytes(std::istream& stream, size_t n) {
        std::vector<uint8_t> buffer(n);
        read_bytes_into_vector(stream, buffer);
        return buffer;
    }

    static std::streamsize consume_n_bytes(std::istream& stream, std::streamsize n) {
        stream.ignore(n);
        return stream.gcount();
    }

    static std::string read_null_terminated_string(std::istream& stream){
        std::string result;
        std::getline(stream, result, '\0');
        return result;
    }
};

}