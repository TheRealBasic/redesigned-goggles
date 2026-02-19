#pragma once

struct Vec2 {
    float x;
    float y;
};

struct TileCoord {
    int x;
    int y;
};

class IsoMath {
public:
    static Vec2 tileToScreen(TileCoord tile, float tileWidth = 128.0F, float tileHeight = 64.0F);
    static TileCoord screenToTile(Vec2 screen, float tileWidth = 128.0F, float tileHeight = 64.0F);
};
