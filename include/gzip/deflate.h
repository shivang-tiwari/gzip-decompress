
#pragma once

#include <iostream>
#include <print>
#include <algorithm>
#include <utils/StreamUtils.h>
#include <utils/BufferUtils.h>

namespace Gzip {

class Deflate {
    public:

    Deflate() {
        std::vector<int> code_lengths(288);
        for(int i = 0;i <= 143;i++){
            code_lengths[i] = 8;
        }
        for(int i = 144;i <= 255;i++){
            code_lengths[i] = 9;
        }
        for(int i = 256;i <= 279;i++){
            code_lengths[i] = 7;
        }
        for(int i = 280;i <= 287;i++){
            code_lengths[i] = 8;
        }
        fixed_literal_decoder.init(code_lengths);

        std::vector<int> distance_code_lengths(32);
        for(int i = 0;i < 32;i++){
            distance_code_lengths[i] = 5;
        }
        fixed_distance_decoder.init(distance_code_lengths);
    }

    void decompress(std::istream& input, std::ostream& output){
        while(input.peek() != std::istream::traits_type::eof()){
            decompress_block(input, output);
            uint32_t CRC32, ISIZE;
            std::vector<uint8_t> footer_bytes = Utils::StreamUtils::read_n_bytes(input, sizeof(CRC32) + sizeof(ISIZE));
            Utils::BufferUtils::deserialize((const char *)footer_bytes.data(), CRC32, ISIZE);
        }
    }
    protected:

    struct bit_reader {
        bit_reader(std::istream& aInput) : input(aInput) { }

        int get_next() {
            if (available_size == 0) {
                int val = input.get();
                next_byte = static_cast<uint8_t>(val);
                available_size = 8;
            }
            --available_size;
            bool bit = (next_byte >> (7 - available_size)) & 1;
            return bit;
        }

        // LSB first
        int get_next_bits(int n){
            int res = 0;
            for(int i = 0;i < n;i++){
                res |= get_next() << i;
            }
            return res;
        }


        // from next byte boundary
        std::vector<uint8_t> read_n_bytes(size_t n) {
            skip_to_byte_boundary();
            return Utils::StreamUtils::read_n_bytes(input, n);
        }

        int get_next_byte(){
            skip_to_byte_boundary();
            return input.get();
        }

        protected:
        std::istream& input;
        uint8_t next_byte = 0;
        size_t available_size = 0;

        void skip_to_byte_boundary() {
            available_size = 0;
            next_byte = 0;
        }

    };

    struct ring_buffer_writer {

        static constexpr int SIZE = 34000; // 34 KB

        ring_buffer_writer(std::ostream& aOutput) : output(aOutput) { }

        void repeat(int distance, int len){
            for(int i = 0;i < len;i++){
                int read_ptr = write_ptr - distance;
                if(read_ptr < 0) read_ptr += SIZE;
                write_one(buffer[read_ptr]);
            }
        }
        
        void write_one(char c){
            buffer[write_ptr] = c;
            output.put(c);
            write_ptr++;
            if(write_ptr == SIZE){
                write_ptr = 0;
            }
        }

        protected:
        std::ostream& output;
        int write_ptr = 0;
        char buffer[SIZE] = {0};

    };

    struct canonical_huffman_decoder {
        void init(const std::vector<int> &code_lengths){
            int max_len = *std::max_element(code_lengths.begin(), code_lengths.end());
            code.assign(1 << max_len, -1);
            length.assign(1 << max_len, 0);

            std::vector<int> bl_count(1 + max_len), next_code(1 + max_len);
            for(int x : code_lengths){
                bl_count[x]++;
            }

            int c = 0;
            bl_count[0] = 0;
            for (int bits = 1; bits <= max_len; bits++) {
                c = (c + bl_count[bits-1]) << 1;
                next_code[bits] = c;
            }
            for(int i = 0;i < (int)code_lengths.size();i++){
                if (code_lengths[i] == 0) continue;
                int code_value = next_code[code_lengths[i]]++;
                code[code_value] = i;
                length[code_value] = code_lengths[i];
            }
        }

        int get_next_symbol(bit_reader &br){
            int yet = 0;
            for(int i = 0;(1 << i) < (int)code.size();i++){
                yet = (yet << 1) | (br.get_next());
                if(length[yet] == i + 1){
                    return code[yet];
                }
            }
            return -1;
        }

        protected:
        std::vector<int> code;
        std::vector<int> length;

    };

    static constexpr int base_length[286] = {
        // 0 - 256: Unused 
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0, // 256
        // 257 - 285
        3,  4,  5,  6,  7,  8,  9,  10, 11, 13, 15, 17, 19, 23, 27, 31,
        35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258
    };

    static constexpr int extra_length_bits[286] = {
        // 0 - 256: Unused
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0, // 256
        // 257 - 285
        0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2,
        3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0
    };

    static constexpr int base_distance[30] = {
        1,    2,    3,    4,    5,    7,    9,    13,   17,   25,
        33,   49,   65,   97,   129,  193,  257,  385,  513,  769,
        1025, 1537, 2049, 3073, 4097, 6145, 8193, 12289, 16385, 24577
    };

    static constexpr int extra_distance_bits[30] = {
        0,  0,  0,  0,  1,  1,  2,  2,  3,  3,
        4,  4,  5,  5,  6,  6,  7,  7,  8,  8,
        9,  9,  10, 10, 11, 11, 12, 12, 13, 13
    };

    canonical_huffman_decoder fixed_literal_decoder;
    canonical_huffman_decoder fixed_distance_decoder;
    canonical_huffman_decoder dynamic_literal_decoder;
    canonical_huffman_decoder dynamic_distance_decoder;

    void decompress_block(std::istream& input, std::ostream& output){
        bit_reader br(input);
        ring_buffer_writer writer(output);
        while(true){
            bool BFINAL  = br.get_next();
            bool BTYPE_0 = br.get_next();
            bool BTYPE_1 = br.get_next();

            if(BTYPE_0 == 0 && BTYPE_1 == 0){
                no_compress_decompression(br, writer);
            }
            else if(BTYPE_0 == 1 && BTYPE_1 == 0){
                // fixed_huffman
                decompress_huffman(br, fixed_literal_decoder, fixed_distance_decoder, writer);
            }
            else if(BTYPE_0 == 0 && BTYPE_1 == 1){
                // dynamic huffman
                get_dynamic_huffman_decoders(br, dynamic_literal_decoder, dynamic_distance_decoder);
                decompress_huffman(br, dynamic_literal_decoder, dynamic_distance_decoder, writer);
            }
            if(BFINAL) break;
        }
    }

    void no_compress_decompression(bit_reader &br, ring_buffer_writer& output){
        uint16_t len, nlen;
        std::vector<uint8_t> len_bytes = br.read_n_bytes(4);
        Utils::BufferUtils::deserialize((const char *)len_bytes.data(), len, nlen);
        for(uint16_t i = 0;i < len;i++){
            output.write_one(static_cast<char>(br.get_next_byte()));
        }
    }

    void decompress_huffman(bit_reader &br, canonical_huffman_decoder &literal_decoder, canonical_huffman_decoder &distance_decoder, 
        ring_buffer_writer& output){
        while(true){
            int next_symbol = literal_decoder.get_next_symbol(br);
            if(next_symbol < 256){
                output.write_one(static_cast<char>(next_symbol));
            }
            else if(next_symbol == 256){
                return;
            }
            else if(next_symbol < 286){
                int len = base_length[next_symbol] + br.get_next_bits(extra_length_bits[next_symbol]);
                int distance_symbol = distance_decoder.get_next_symbol(br);
                int distance = base_distance[distance_symbol] + br.get_next_bits(extra_distance_bits[distance_symbol]);
                output.repeat(distance, len);
            }
        }
    }

    void get_dynamic_huffman_decoders(bit_reader &br, canonical_huffman_decoder &literal_decoder, 
        canonical_huffman_decoder &distance_decoder){

        int HLIT  = br.get_next_bits(5); 
        int HDIST = br.get_next_bits(5); 
        int HCLEN = br.get_next_bits(4); 

        int num_lit_codes  = HLIT + 257;
        int num_dist_codes = HDIST + 1;
        int num_cl_codes   = HCLEN + 4;

        std::vector<int> code_lengths(19);
        const int cl_order[19] = {
            16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15
        };

        for (int i = 0; i < num_cl_codes; ++i) {
            code_lengths[cl_order[i]] = br.get_next_bits(3);
        }

        canonical_huffman_decoder code_huffman_tree;
        code_huffman_tree.init(code_lengths);

        int total_lengths = num_lit_codes + num_dist_codes;
        std::vector<int> all_lengths;
        all_lengths.reserve(total_lengths);

        while((int)all_lengths.size() < total_lengths){
            int next_symbol = code_huffman_tree.get_next_symbol(br);
            if (next_symbol <= 15) {
                all_lengths.push_back(next_symbol);
            } 
            else if (next_symbol == 16) {
                int repeat = 3 + br.get_next_bits(2);
                int prev = all_lengths.back();
                while (repeat--) all_lengths.push_back(prev);
            } 
            else if (next_symbol == 17) {
                int repeat = 3 +  br.get_next_bits(3);
                while (repeat--) all_lengths.push_back(0);
            } 
            else if (next_symbol == 18) {
                int repeat = 11 + br.get_next_bits(7);
                while (repeat--) all_lengths.push_back(0);
            }
        }

        std::vector<int> lit_lens(all_lengths.begin(), all_lengths.begin() + num_lit_codes);
        std::vector<int> dist_lens(all_lengths.begin() + num_lit_codes, all_lengths.end());

        literal_decoder.init(lit_lens);
        distance_decoder.init(dist_lens);
    }

};

} // namespace Gzip