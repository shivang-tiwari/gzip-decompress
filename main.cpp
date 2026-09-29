#include <gzip/validate.h>
#include <gzip/deflate.h>
#include <fstream>

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "Error: Missing file name argument.\n";
        std::cerr << "Usage: " << argv[0] << " <input-filename> <output-filename>\n";
        return 1;
    }

    std::string input = argv[1];
    std::ifstream infile(input, std::ios::binary);

    if (!infile.is_open()) {
        std::cerr << "Failed to open input file: " << input << '\n';
        return 1;
    }

    std::string output = argv[2];
    std::ofstream outfile(output);

    if (!outfile.is_open()) {
        std::cerr << "Error opening output file!" << std::endl;
        return 1;
    }

    bool header = Gzip::ValidateHeader::validate(infile);

    if(!header){
        std::cerr << "Invalid header" << std::endl;
        return 1;
    }

    Gzip::Deflate deflate;

    if(!deflate.decompress(infile, outfile)){
        std::cerr << "Decompress failed" << std::endl;
        return 1;
    }

    return 0;
}