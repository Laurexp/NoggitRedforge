#pragma once
#include "../erosion/TerrainTypes.h"
#include <vector>
#include <utility>

struct RoadParams {
    float slope_penalty = 50.0f;
    float river_penalty = 10.0f;
    int road_radius = 2;
    float flatten_amount = 0.8f; // 0.0 to 1.0
};

class RoadGenerator {
public:
    static std::vector<bool> generate(
        Heightmap& map, 
        const std::vector<bool>& river_mask, 
        const std::vector<std::pair<int, int>>& waypoints,
        const RoadParams& params = RoadParams()
    );
};
