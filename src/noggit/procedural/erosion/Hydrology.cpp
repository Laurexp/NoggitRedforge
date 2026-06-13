#include "Hydrology.h"
#include <queue>
#include <algorithm>
#include <cmath>
#include <iostream>

struct Cell {
    int x, y;
    float z;
    bool operator>(const Cell& other) const {
        return z > other.z; // Min-heap based on elevation
    }
};

std::vector<bool> Hydrology::fillSinksWangLiu(Heightmap& map) {
    int width = map.width;
    int height = map.height;
    
    std::vector<float> spill(width * height, std::numeric_limits<float>::infinity());
    std::vector<bool> isLake(width * height, false);
    std::priority_queue<Cell, std::vector<Cell>, std::greater<Cell>> pq;

    // Push boundary cells
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            if (x == 0 || x == width - 1 || y == 0 || y == height - 1) {
                int idx = y * width + x;
                spill[idx] = map.data[idx];
                pq.push({x, y, spill[idx]});
            }
        }
    }

    int dx[] = {-1, 0, 1, -1, 1, -1, 0, 1};
    int dy[] = {-1, -1, -1, 0, 0, 1, 1, 1};

    while (!pq.empty()) {
        Cell c = pq.top();
        pq.pop();

        if (c.z > spill[c.y * width + c.x]) continue;

        for (int i = 0; i < 8; ++i) {
            int nx = c.x + dx[i];
            int ny = c.y + dy[i];

            if (nx >= 0 && nx < width && ny >= 0 && ny < height) {
                int nIdx = ny * width + nx;
                float oldSpill = spill[nIdx];
                float newSpill = std::max(c.z, map.data[nIdx]);
                
                if (oldSpill == std::numeric_limits<float>::infinity() || newSpill < oldSpill) {
                    spill[nIdx] = newSpill;
                    pq.push({nx, ny, newSpill});
                }
            }
        }
    }

    // Apply spill elevations back to map and mark lakes
    for (int i = 0; i < width * height; ++i) {
        if (spill[i] > map.data[i] + 0.0001f) {
            map.data[i] = spill[i];
            isLake[i] = true;
        }
    }

    return isLake;
}

Heightmap Hydrology::calculateFlowAccumulation(const Heightmap& map) {
    int width = map.width;
    int height = map.height;
    
    std::vector<int> flowDir(width * height, -1);
    std::vector<float> accum(width * height, 1.0f); // initial unit of water
    
    std::vector<int> sortedIndices(width * height);
    for (int i = 0; i < width * height; ++i) {
        sortedIndices[i] = i;
    }
    
    // Sort indices by elevation (descending)
    std::sort(sortedIndices.begin(), sortedIndices.end(), [&map](int a, int b) {
        return map.data[a] > map.data[b];
    });

    int dx[] = {-1, 0, 1, -1, 1, -1, 0, 1};
    int dy[] = {-1, -1, -1, 0, 0, 1, 1, 1};

    // Calculate D8 flow directions
    for (int i = 0; i < width * height; ++i) {
        int x = i % width;
        int y = i / width;
        
        float minZ = map.data[i];
        int bestDir = -1;
        float maxDrop = 0.0f;

        for (int d = 0; d < 8; ++d) {
            int nx = x + dx[d];
            int ny = y + dy[d];
            if (nx >= 0 && nx < width && ny >= 0 && ny < height) {
                int nIdx = ny * width + nx;
                float drop = map.data[i] - map.data[nIdx];
                // Diagonal distance is sqrt(2), so slope is drop / sqrt(2)
                float dist = (dx[d] == 0 || dy[d] == 0) ? 1.0f : 1.41421356f;
                float slope = drop / dist;
                
                if (slope > maxDrop) {
                    maxDrop = slope;
                    bestDir = nIdx;
                }
            }
        }
        flowDir[i] = bestDir;
    }

    // Accumulate flow from top to bottom
    float maxAccum = 1.0f;
    for (int idx : sortedIndices) {
        int next = flowDir[idx];
        if (next != -1) {
            accum[next] += accum[idx];
            if (accum[next] > maxAccum) maxAccum = accum[next];
        }
    }

    // Normalize and log scale flow for better visualization
    Heightmap flowMap(width, height);
    if (maxAccum > 1.0f) {
        float logMax = std::log1p(maxAccum);
        for (int i = 0; i < width * height; ++i) {
            flowMap.data[i] = std::log1p(accum[i]) / logMax;
        }
    } else {
        std::fill(flowMap.data.begin(), flowMap.data.end(), 0.0f);
    }
    
    return flowMap;
}
