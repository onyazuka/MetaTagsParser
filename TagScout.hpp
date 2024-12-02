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
    void dump(const std::filesystem::path& path);
private:
    MapT framePathMap;
};

#endif // TAGSCOUT_H
