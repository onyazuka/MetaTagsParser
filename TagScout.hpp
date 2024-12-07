#ifndef TAGSCOUT_H
#define TAGSCOUT_H
#include <map>
#include <string>
#include <filesystem>
#include <list>

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

#endif // TAGSCOUT_H
