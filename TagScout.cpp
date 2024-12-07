#include "TagScout.hpp"
#include <fstream>
#include "ID3V2Parser.hpp"
#include "Mp3FrameParser.hpp"
#include "FlacTagParser.hpp"

namespace fs = std::filesystem;
using namespace tag::id3v2;
using namespace tag::flac;
using namespace mp3;
using namespace tag;

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
            std::unique_ptr<Tag> parser;
            if (extension == ".mp3"){
                parser.reset(new ID3V2Parser(ifs));
            }
            else {
                parser.reset(new FlacTagParser(ifs));
            }
            for (const auto& frame: parser->getExtractor()->frameTitles()) {
                framePathMap[frame].push_back(entry.path().string());
            }
            if (extension == ".mp3") {
                /*if (extractor.version() == 4) {
                    framePathMap["v4"].push_back(entry.path().string());
                }*/
                songDurationMap[entry.path()] = getMp3FileDuration(ifs);
            }
            else {
                auto p = dynamic_cast<FlacTagParser*>(parser.get());
                songDurationMap[entry.path()] = p->StreamInfo().totalSamples / p->StreamInfo().sampleRate;
            }

            /*mp3::Mp3FrameParser mp3FrameParser(ifs);
            if (mp3FrameParser.isVBR()) {
                framePathMap["VBR"].push_back(entry.path().string());
            }
            else {
                framePathMap["CBR"].push_back(entry.path().string());
            }*/
        }
        catch (NoTagException&) {
            // not a error, just no tag
            continue;
        }
        catch (UnknownTagException&) {
            // not a error, just unknown tag
            framePathMap["unknown"].push_back(entry.path().string());
            continue;
        }
        catch (InvalidTagException&) {
            // tag is invalid, but some data may be ok
            continue;
        }
        catch (Mp3FrameParser::EOFException&) {
            continue;
        }
        catch (Mp3FrameParser::NoFrameException&) {
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

void TagScout::dumpDurations(const std::filesystem::path& path) {
    std::ofstream ofs(path);
    if (!ofs) {
        return;
    }
    for (const auto& [path, duration] : songDurationMap) {
        ofs << duration << ": " << path << std::endl;
    }
}
