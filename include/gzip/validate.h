
#pragma once

#include <iostream>
#include <iterator>
#include <algorithm>
#include <print>
#include <utils/BufferUtils.h>
#include <utils/StreamUtils.h>

namespace Gzip {
/*
A compliant decompressor must check ID1, ID2, and CM, and
provide an error indication if any of these have incorrect
values.  It must examine FEXTRA/XLEN, FNAME, FCOMMENT and FHCRC
at least so it can skip over the optional fields if they are
present.  It need not examine any other part of the header or
trailer; in particular, a decompressor may ignore FTEXT and OS
and always produce binary output, and still be compliant.  A
compliant decompressor must give an error indication if any
reserved bit is non-zero, since such a bit could indicate the
presence of a new field that would cause subsequent data to be
interpreted incorrectly.


*/
class ValidateHeader {

    public:
    
    static bool validate(std::istream& stream){
        auto [header, err] = getHeader(stream);
        if(!err.empty()){
            std::println(stderr, "Error : {}", err);
            return false;
        }
        return true;
    }

    protected:

    struct Header {
        // fixed
        uint32_t MTIME;
        uint8_t  ID1,ID2,CM,FLG,XFL,OS;

        // variable
        uint16_t XLEN = 0;
        uint16_t CRC16 = 0;
        std::string FILE_NAME;
        std::string FILE_COMMENT;

        static size_t fixed_size(){
            return sizeof(MTIME) + sizeof(ID1) + sizeof(ID2) + sizeof(CM) + sizeof(FLG) + sizeof(XFL) + sizeof(OS);
        }
        void deserialize(const char* buffer){
            Utils::BufferUtils::deserialize(buffer, ID1, ID2, CM, FLG, MTIME, XFL, OS);
        }

        void print() const {
            std::println("Header Details:");
            std::println("  MTIME:        {}", MTIME);
            std::println("  ID1:          {:#04x}", static_cast<unsigned int>(ID1));
            std::println("  ID2:          {:#04x}", static_cast<unsigned int>(ID2));
            std::println("  CM:           {}", static_cast<unsigned int>(CM));
            std::println("  FLG:          {:#04x}", static_cast<unsigned int>(FLG));
            std::println("  XFL:          {}", static_cast<unsigned int>(XFL));
            std::println("  OS:           {}", static_cast<unsigned int>(OS));
            std::println("  XLEN:         {}", XLEN);
            std::println("  CRC16:        {:#06x}", CRC16);
            std::println("  FILE_NAME:    \"{}\"", FILE_NAME);
            std::println("  FILE_COMMENT: \"{}\"", FILE_COMMENT);
        }

        bool FTEXT()    {return FLG & 1;}
        bool FHCRC()    {return FLG & 2;}
        bool FEXTRA()   {return FLG & 4;}
        bool FNAME()    {return FLG & 8;}
        bool FCOMMENT() {return FLG & 16;}

        bool reserve()  {return FLG & (32 | 64 | 128);}

    };

    static std::pair<Header, std::string> getHeader(std::istream& stream){
        Header header;
        std::vector<uint8_t> headerBytes = Utils::StreamUtils::read_n_bytes(stream, Header::fixed_size());

        if(headerBytes.size() != Header::fixed_size()){
            return {header, "Insufficient size for fixed header"};
        }
        header.deserialize((const char *)headerBytes.data());

        if(header.ID1 != 31 || header.ID2 != 139){
            return {header, "Invalid ID1/2 in header"};
        }

        if(header.CM != 8){
            return {header, "Invalid compression method in header"};
        }

        if(header.FEXTRA()){
            headerBytes = Utils::StreamUtils::read_n_bytes(stream, sizeof(header.XLEN));
            if(headerBytes.size() != sizeof(header.XLEN)){
                return {header, "FEXTRA is set but XLEN not present"};
            }
            Utils::BufferUtils::deserialize((const char *)headerBytes.data(), header.XLEN);
            int extra_len = Utils::StreamUtils::consume_n_bytes(stream, header.XLEN);
            if(header.XLEN != extra_len){
                return {header, "FEXTRA is set but XLEN bytes are not present"};
            }
        }
        if(header.FNAME()){
            header.FILE_NAME =  Utils::StreamUtils::read_null_terminated_string(stream);
        }
        if(header.FCOMMENT()){
            header.FILE_COMMENT =  Utils::StreamUtils::read_null_terminated_string(stream);
        }
        if(header.FHCRC()){
            headerBytes = Utils::StreamUtils::read_n_bytes(stream, sizeof(header.CRC16));
            if(headerBytes.size() != sizeof(header.CRC16)){
                return {header, "FHCRC is set but CRC16 not present"};
            }
            Utils::BufferUtils::deserialize((const char *)headerBytes.data(), header.CRC16);
        }
        if(header.reserve()){
            return {header, "Reserved bits must be unset in the header"};
        }
        return {header, ""};
    }
};

} // namespace Gzip