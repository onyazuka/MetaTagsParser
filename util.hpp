#ifndef UTIL_HPP
#define UTIL_HPP
#include <map>
#include <string>
#include <filesystem>
#include <list>
#include "ID3V2Parser.hpp"

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

#endif // UTIL_HPP
