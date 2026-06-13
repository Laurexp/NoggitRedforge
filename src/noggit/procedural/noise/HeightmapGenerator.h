#pragma once

#include "../erosion/TerrainTypes.h"
#include <vector>

class HeightmapGenerator {
public:
    // Generates a heightmap from a list of configured noise layers and modifiers.
    static Heightmap generate(int baseSeed, int offsetX, int offsetY, int width, int height, const std::vector<NoiseLayer>& layers, const ModifierParams& modifiers);
};
