#pragma once
#include "../erosion/TerrainTypes.h"
#include <vector>

class RiverGenerator {
public:
    // Generates rivers, modifies heightmap by carving, returns river mask (true for river pixels)
    static std::vector<bool> generate(Heightmap& map, int num_rivers, int seed = 12345);
};
