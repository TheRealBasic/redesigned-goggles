#pragma once

#include <string>
#include <vector>

class Map {
public:
    bool loadFromAsciiFile(const std::string& path);
    bool isBlocked(int x, int y) const;
    int width() const;
    int height() const;

private:
    std::vector<std::string> m_rows;
};
