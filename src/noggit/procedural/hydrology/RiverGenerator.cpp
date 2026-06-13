#include "RiverGenerator.h"
#include <random>
#include <algorithm>
#include <cmath>
#include <iostream>

struct Point {
    int x, y;
};

std::vector<bool> RiverGenerator::generate(Heightmap& map, int num_rivers, int seed) {
    std::vector<bool> river_mask(map.width * map.height, false);
    
    if (num_rivers <= 0) return river_mask;

    std::mt19937 rng(seed);
    
    // Find min and max height to determine the top 20%
    float min_h = 1.0f;
    float max_h = 0.0f;
    for (float h : map.data) {
        if (h < min_h) min_h = h;
        if (h > max_h) max_h = h;
    }
    
    float threshold = min_h + 0.8f * (max_h - min_h);
    
    std::vector<Point> candidates;
    for (int y = 0; y < map.height; ++y) {
        for (int x = 0; x < map.width; ++x) {
            if (map.at(x, y) >= threshold) {
                candidates.push_back({x, y});
            }
        }
    }
    
    if (candidates.empty()) {
        return river_mask;
    }
    
    std::vector<std::vector<Point>> all_rivers;

    for (int r = 0; r < num_rivers; ++r) {
        std::uniform_int_distribution<size_t> dist(0, candidates.size() - 1);
        Point current = candidates[dist(rng)];
        
        std::vector<Point> path;
        path.push_back(current);
        
        while (true) {
            Point next_pt = current;
            float lowest_h = map.at(current.x, current.y);
            bool found_lower = false;
            
            // Check 8 neighbors for steepest descent
            for (int dy = -1; dy <= 1; ++dy) {
                for (int dx = -1; dx <= 1; ++dx) {
                    if (dx == 0 && dy == 0) continue;
                    
                    int nx = current.x + dx;
                    int ny = current.y + dy;
                    
                    if (nx >= 0 && nx < map.width && ny >= 0 && ny < map.height) {
                        float h = map.at(nx, ny);
                        if (h < lowest_h) {
                            lowest_h = h;
                            next_pt = {nx, ny};
                            found_lower = true;
                        }
                    }
                }
            }
            
            if (!found_lower) {
                break; // Hit a pit or flat area
            }
            
            current = next_pt;
            path.push_back(current);
            
            if (current.x == 0 || current.x == map.width - 1 || current.y == 0 || current.y == map.height - 1) {
                break; // Hit boundary
            }
        }
        
        if (path.size() > 10) { // Only keep rivers that are reasonably long
            all_rivers.push_back(path);
        }
    }
    
    // Carve heightmap (gaussian-smoothed)
    // We create a temporary carve map to avoid accumulating depth where river curves around itself
    std::vector<float> carve_map(map.width * map.height, 0.0f);
    int radius = 4;
    float max_depth = 0.03f;
    
    for (const auto& path : all_rivers) {
        for (const auto& p : path) {
            // Mark immediate neighbors for a slightly wider mask for alphamap
            for (int dy = -1; dy <= 1; ++dy) {
                for (int dx = -1; dx <= 1; ++dx) {
                    int nx = p.x + dx;
                    int ny = p.y + dy;
                    if (nx >= 0 && nx < map.width && ny >= 0 && ny < map.height) {
                        river_mask[ny * map.width + nx] = true;
                    }
                }
            }

            // Calculate carve depths
            for (int dy = -radius; dy <= radius; ++dy) {
                for (int dx = -radius; dx <= radius; ++dx) {
                    int nx = p.x + dx;
                    int ny = p.y + dy;
                    
                    if (nx >= 0 && nx < map.width && ny >= 0 && ny < map.height) {
                        float dist_sq = static_cast<float>(dx * dx + dy * dy);
                        float r_sq = static_cast<float>(radius * radius);
                        if (dist_sq <= r_sq) {
                            // Gaussian-like falloff
                            float falloff = std::exp(-dist_sq / (r_sq * 0.5f));
                            float carve = max_depth * falloff;
                            
                            int idx = ny * map.width + nx;
                            if (carve > carve_map[idx]) {
                                carve_map[idx] = carve;
                            }
                        }
                    }
                }
            }
        }
    }
    
    // Apply carve map
    for (int i = 0; i < map.width * map.height; ++i) {
        map.data[i] = (std::max)(0.0f, map.data[i] - carve_map[i]);
    }
    
    return river_mask;
}
