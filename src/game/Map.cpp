#include "game/Map.hpp"

#include <fstream>

bool Map::loadFromAsciiFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        return false;
    }

    m_rows.clear();
    std::string line;
    while (std::getline(file, line)) {
        if (!line.empty()) {
            m_rows.push_back(line);
        }
    }
    return !m_rows.empty();
}

bool Map::isBlocked(int x, int y) const {
    if (y < 0 || x < 0 || y >= static_cast<int>(m_rows.size())) {
        return true;
    }
    if (x >= static_cast<int>(m_rows[y].size())) {
        return true;
    }
    return m_rows[y][x] == '#';
}

int Map::width() const {
    if (m_rows.empty()) {
        return 0;
    }
    return static_cast<int>(m_rows.front().size());
}

int Map::height() const {
    return static_cast<int>(m_rows.size());
}
