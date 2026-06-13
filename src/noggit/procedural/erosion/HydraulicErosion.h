#pragma once

#include "TerrainTypes.h"

struct ErosionParams {
    int num_droplets = 250000;
    int max_steps = 60;
    float inertia = 0.1f;
    float sediment_capacity = 8.0f;
    float min_sediment_capacity = 0.05f;
    float erode_speed = 0.5f;
    float deposit_speed = 0.1f;
    float evaporate_speed = 0.005f;
    float gravity = 6.0f;
    float initial_water = 1.0f;
    float initial_velocity = 1.0f;
    
    // Thermal weathering (Slippage)
    int thermal_iterations = 0;
    float thermal_slope_threshold = 0.03f;
};

class HydraulicErosion {
public:
    static void erode(Heightmap& map, const ErosionParams& params, int seed = 12345);
};
