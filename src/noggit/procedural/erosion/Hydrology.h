#pragma once
#include "TerrainTypes.h"
#include <vector>

class Hydrology {
public:
    // Implements Wang & Liu (2006) Sink Filling algorithm.
    // Fills all closed depressions (sinks) in the heightmap to their spill elevation.
    // Creates flat lakes and ensures every cell has a path to the map edge.
    // Returns a boolean mask (same size as Heightmap) where true indicates a filled cell (lake/sink).
    static std::vector<bool> fillSinksWangLiu(Heightmap& map);

    // Calculates D8 Flow Accumulation.
    // Returns a map representing how much water flows through each cell.
    // Flow accumulation values are normalized between 0.0 and 1.0 (with log scaling applied for better visibility).
    static Heightmap calculateFlowAccumulation(const Heightmap& map);
};
