#include <iostream>
#include <cassert>

using namespace std;
#include "ID3V2Parser.hpp"
#include "util.hpp"

void testUtfConverters() {
    std::string s1 = "neko";
    std::string s2 = "虹のコンキスタドール";
    std::string s3 = "ࠁ and ࠂ";
    std::string s4 = "🌉 🌉 🌉";
    auto ws1 = utf8ToUtf16(s1.data(), s1.size());
    auto ws2 = utf8ToUtf16(s2.data(), s2.size());
    auto ws3 = utf8ToUtf16(s3.data(), s3.size());
    auto ws4 = utf8ToUtf16(s4.data(), s4.size());
    auto s1_1 = utf16ToUtf8((char*)ws1.data(), ws1.size() * 2);
    auto s2_1 = utf16ToUtf8((char*)ws2.data(), ws2.size() * 2);
    auto s3_1 = utf16ToUtf8((char*)ws3.data(), ws3.size() * 2);
    auto s4_1 = utf16ToUtf8((char*)ws4.data(), ws4.size() * 2);
    assert(s1 == s1_1);
    assert(s2 == s2_1);
    assert(s3 == s3_1);
    assert(s4 == s4_1);
}

void testParser() {
    testUtfConverters();
}

int main()
{

    testParser();
    TagScout scout("/media/onyazuka/New SSD/music");
    const auto& map = scout.map();
    //scout.dump("/home/onyazuka/taginfo.txt");
    try {
        std::string home = "/home/onyazuka/";
        std::string path = home + "鈴木このみ アスタロア.mp3";
        //std::string path = "/media/onyazuka/New SSD/music/WHY@DOLL 1月21日発売の4thシングル「サンライズ！～君がくれた希望～」MV (mp3cut.ru).mp3";
        std::ifstream ifs(path, std::ios_base::binary);
        if (!ifs) {
            throw std::runtime_error("error opening file");
        }
        ID3V2Extractor extractor(ifs);
        cout << "ok" << endl;

        std::unordered_map<std::string, std::string> tags;
        auto frames = extractor.frames();
        ID3V2Parser parser(std::move(extractor));
        for (auto& [title, frame] : frames) {
            if(title[0] == 'T') {
                tags[title] = std::get<1>(parser.Textual(title));
            }
            else if ((title[0] == 'W') && (title != "WXXX")) {
                tags[title] = std::get<0>(parser.WUrl(title));
            }
        }
        APICReader::ResultType apic = parser.APIC();
        auto wxxx = parser.WXXX();
        auto comm = parser.COMM();
        for (auto& [title, tag] : tags) {
            std::cout << title << ": " ;
            std::cout << tag << "\n";
        }
    }
    catch (std::runtime_error& err) {
        cout << "Error occured: " << err.what() << endl;
    }

    return 0;
}
