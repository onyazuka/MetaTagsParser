#ifndef TAGSCOUT_H
#define TAGSCOUT_H
#include <map>
#include <unordered_map>
#include <string>
#include <filesystem>
#include <list>
#include <variant>
#include "ID3V2Parser.hpp"
#include "Mp3FrameParser.hpp"
#include "FlacTagParser.hpp"

/*
    for testing purposes
*/
class TagScout {
public:
    using MapT = std::map<std::string, std::list<std::string>>;
    TagScout(const std::filesystem::path& path);
    inline const MapT map() const {
        return framePathMap;
    }
    inline const std::map<std::string, size_t>& durations() const {
        return songDurationMap;
    }
    void dump(const std::filesystem::path& path);
    void dumpDurations(const std::filesystem::path& path);
private:
    MapT framePathMap;
    std::map<std::string, size_t> songDurationMap;
};

/*
    Function for extracting metainfo from a file.
    Populates a metainfo map.
    Keys:
        title
        album
        artist,
        images
        durationMs
*/
using MetainfoData = std::variant<std::string, std::vector<tag::user::APICUserData>>;
struct GetMetaInfoConfig {
    bool textual;
    bool duration;
    bool images;
};

std::unordered_map<std::string, MetainfoData> getMetainfo(const std::filesystem::path& path, const GetMetaInfoConfig& config);

#endif // TAGSCOUT_H
