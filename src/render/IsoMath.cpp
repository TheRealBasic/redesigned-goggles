#include "render/IsoMath.hpp"

#include <cmath>

Vec2 IsoMath::tileToScreen(TileCoord tile, float tileWidth, float tileHeight) {
    const float halfWidth = tileWidth * 0.5F;
    const float halfHeight = tileHeight * 0.5F;
    return {
        static_cast<float>(tile.x - tile.y) * halfWidth,
        static_cast<float>(tile.x + tile.y) * halfHeight,
    };
}

TileCoord IsoMath::screenToTile(Vec2 screen, float tileWidth, float tileHeight) {
    const float halfWidth = tileWidth * 0.5F;
    const float halfHeight = tileHeight * 0.5F;

    const float rawX = (screen.x / halfWidth + screen.y / halfHeight) * 0.5F;
    const float rawY = (screen.y / halfHeight - screen.x / halfWidth) * 0.5F;

    return {
        static_cast<int>(std::floor(rawX)),
        static_cast<int>(std::floor(rawY)),
    };
}
