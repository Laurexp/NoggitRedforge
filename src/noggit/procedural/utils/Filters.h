#pragma once
#include "../erosion/TerrainTypes.h"

class Filters {
public:
    // Applies a fast box blur to the given heightmap.
    static void fastBoxBlur(Heightmap& map, int radius);
};
