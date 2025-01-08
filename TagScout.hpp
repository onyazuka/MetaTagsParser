#ifndef TAGSCOUT_H
#define TAGSCOUT_H
#include <map>
#include <unordered_map>
#include <string>
#include <filesystem>
#include <list>

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
        artist
        durationMs
*/
std::unordered_map<std::string, std::string> getMetainfo(const std::filesystem::path& path);

#endif // TAGSCOUT_H
