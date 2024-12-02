#include "TagScout.hpp"
#include <fstream>
#include "ID3V2Parser.hpp"

namespace fs = std::filesystem;
using namespace tag::id3v2;

TagScout::TagScout(const std::filesystem::path& path) {
    for (const fs::directory_entry& entry : fs::recursive_directory_iterator(path)) {
        try {
            if (!entry.is_regular_file()) {
                continue;
            }
            auto extension = entry.path().extension();
            if (!(extension == ".mp3" || extension == ".flac")) {
                continue;
            }
            std::ifstream ifs(entry.path(), std::ios_base::binary);
            if (!ifs) {
                continue;
            }
            ID3V2Extractor extractor(ifs);
            const auto& frames = extractor.frames();
            for (const auto& frame: frames) {
                framePathMap[frame.first].push_back(entry.path().string());
            }
            if (extractor.version() == 4) {
                framePathMap["v4"].push_back(entry.path().string());
            }
        }
        catch (ID3V2Extractor::NoTagException) {
            // not a error, just no tag
            continue;
        }
        catch (ID3V2Extractor::UnknownTagException) {
            // not a error, just unknown tag
            framePathMap["unknown"].push_back(entry.path().string());
            continue;
        }
        catch (ID3V2Extractor::InvalidTagException) {
            // tag is invalid, but some data may be ok
            continue;
        }
        catch (...) {
            framePathMap["error"].push_back(entry.path().string());
        }
    }
}

void TagScout::dump(const std::filesystem::path& path) {
    std::ofstream ofs(path);
    if (!ofs) {
        return;
    }
    for (const auto& [frame, songs] : framePathMap) {
        ofs << frame << "\n";
        for (const auto& song : songs) {
            ofs << "\t" << song << "\n";
        }
    }
}
