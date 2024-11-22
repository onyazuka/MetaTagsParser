#include <iostream>

using namespace std;
#include "ID3V2Parser.hpp"

int main()
{
    try {
        std::string path = "/home/onyazuka/01.Bokusatsu_Tenshi_Dokuro-chan.mp3";
        std::ifstream ifs(path, std::ios_base::binary);
        if (!ifs) {
            throw std::runtime_error("error opening file");
        }
        ID3V2Extractor extractor(ifs);
        cout << "ok" << endl;
    }
    catch (std::runtime_error& err) {
        cout << "Error occured: " << err.what() << endl;
    }

    return 0;
}
