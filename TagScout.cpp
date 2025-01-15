#include "TagScout.hpp"
#include <fstream>
#include <algorithm>

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
            std::string extension = path.extension().string();
            std::transform(extension.begin(), extension.end(), extension.begin(), [](char c){ return std::tolower(c); });
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
                songDurationMap[entry.path().string()] = getMp3FileDuration(ifs);
            }
            else {
                auto p = dynamic_cast<FlacTagParser*>(parser.get());
                songDurationMap[entry.path().string()] = p->StreamInfo().totalSamples / p->StreamInfo().sampleRate;
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



std::unordered_map<std::string, MetainfoData> getMetainfo(const std::filesystem::path& path, const GetMetaInfoConfig& config) {

    if (!std::filesystem::is_regular_file(path)) {
        throw NoTagException{};
    }
    std::string extension = path.extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(), [](char c){ return std::tolower(c); });
    if (!(extension == ".mp3" || extension == ".flac")) {
        return {};
    }
    std::ifstream ifs(path, std::ios_base::binary);
    if (!ifs) {
        return {};
    }
    try {
        std::unique_ptr<Tag> parser;
        try {
            if (extension == ".mp3"){
                parser.reset(new ID3V2Parser(ifs));
            }
            else {
                parser.reset(new FlacTagParser(ifs));
            }
        }
        catch (...) {}
        if (!parser) {
            return {};
        }
        std::unordered_map<std::string, MetainfoData> metainfo;
        if (config.textual) {
            metainfo["title"] = parser->songTitle();
            metainfo["album"] = parser->album();
            metainfo["artist"] = parser->artist();
            metainfo["year"] = parser->year();
            metainfo["trackNumber"] = parser->trackNumber();
            metainfo["comment"] = parser->comment();
        }
        if (config.duration) {
            metainfo["durationMs"] = std::to_string(parser->durationMs());
        }
        if (config.images) {
            metainfo["images"] = parser->image();
        }
        return metainfo;
    }
    catch (...) {
        return {};
    }
}
