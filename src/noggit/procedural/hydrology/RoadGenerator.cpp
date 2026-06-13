#include "RoadGenerator.h"
#include <queue>
#include <cmath>
#include <algorithm>
#include <limits>

struct AStarNode {
    int x, y;
    float f_score;
    bool operator>(const AStarNode& other) const {
        return f_score > other.f_score;
    }
};

static float distance(int x1, int y1, int x2, int y2) {
    return std::sqrt(static_cast<float>((x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2)));
}

static std::vector<std::pair<int, int>> findPath(
    const Heightmap& map, 
    const std::vector<bool>& river_mask, 
    std::pair<int, int> start, 
    std::pair<int, int> goal,
    const RoadParams& params) 
{
    int width = map.width;
    int height = map.height;
    
    std::vector<float> g_score(width * height, std::numeric_limits<float>::infinity());
    std::vector<int> came_from(width * height, -1);
    
    std::priority_queue<AStarNode, std::vector<AStarNode>, std::greater<AStarNode>> open_set;
    
    int start_idx = start.second * width + start.first;
    g_score[start_idx] = 0;
    open_set.push({start.first, start.second, distance(start.first, start.second, goal.first, goal.second)});
    
    while (!open_set.empty()) {
        AStarNode current = open_set.top();
        open_set.pop();
        
        if (current.x == goal.first && current.y == goal.second) {
            std::vector<std::pair<int, int>> path;
            int curr_idx = goal.second * width + goal.first;
            while (curr_idx != -1) {
                path.push_back({curr_idx % width, curr_idx / width});
                curr_idx = came_from[curr_idx];
            }
            std::reverse(path.begin(), path.end());
            return path;
        }
        
        int curr_idx = current.y * width + current.x;
        
        for (int dy = -1; dy <= 1; ++dy) {
            for (int dx = -1; dx <= 1; ++dx) {
                if (dx == 0 && dy == 0) continue;
                
                int nx = current.x + dx;
                int ny = current.y + dy;
                
                if (nx >= 0 && nx < width && ny >= 0 && ny < height) {
                    int neighbor_idx = ny * width + nx;
                    
                    float dist_cost = (dx == 0 || dy == 0) ? 1.0f : 1.4142f;
                    float height_diff = std::abs(map.at(nx, ny) - map.at(current.x, current.y));
                    float river_cost = river_mask[neighbor_idx] ? params.river_penalty : 0.0f;
                    
                    float tentative_g = g_score[curr_idx] + dist_cost + params.slope_penalty * height_diff + river_cost;
                    
                    if (tentative_g < g_score[neighbor_idx]) {
                        came_from[neighbor_idx] = curr_idx;
                        g_score[neighbor_idx] = tentative_g;
                        float f_score = tentative_g + distance(nx, ny, goal.first, goal.second);
                        open_set.push({nx, ny, f_score});
                    }
                }
            }
        }
    }
    
    return {};
}

static float catmullRom1D(float p0, float p1, float p2, float p3, float t) {
    float t2 = t * t;
    float t3 = t2 * t;
    return 0.5f * ((2.0f * p1) +
                   (-p0 + p2) * t +
                   (2.0f * p0 - 5.0f * p1 + 4.0f * p2 - p3) * t2 +
                   (-p0 + 3.0f * p1 - 3.0f * p2 + p3) * t3);
}

static std::pair<float, float> catmullRom2D(
    std::pair<float, float> p0, 
    std::pair<float, float> p1, 
    std::pair<float, float> p2, 
    std::pair<float, float> p3, 
    float t) 
{
    return {
        catmullRom1D(p0.first, p1.first, p2.first, p3.first, t),
        catmullRom1D(p0.second, p1.second, p2.second, p3.second, t)
    };
}

struct SmoothPoint {
    int x, y;
    float height;
};

std::vector<bool> RoadGenerator::generate(
    Heightmap& map, 
    const std::vector<bool>& river_mask, 
    const std::vector<std::pair<int, int>>& waypoints,
    const RoadParams& params) 
{
    std::vector<bool> road_mask(map.width * map.height, false);
    
    if (waypoints.size() < 2) return road_mask;
    
    std::vector<std::pair<int, int>> full_path;
    
    for (size_t i = 0; i < waypoints.size() - 1; ++i) {
        auto path = findPath(map, river_mask, waypoints[i], waypoints[i+1], params);
        if (path.empty()) continue;
        
        if (!full_path.empty() && full_path.back() == path.front()) {
            full_path.insert(full_path.end(), path.begin() + 1, path.end());
        } else {
            full_path.insert(full_path.end(), path.begin(), path.end());
        }
    }
    
    if (full_path.size() < 2) return road_mask;

    std::vector<std::pair<float, float>> control_pts;
    std::vector<float> control_heights;
    
    for (size_t i = 0; i < full_path.size(); i += 3) {
        control_pts.push_back({full_path[i].first, full_path[i].second});
        control_heights.push_back(map.at(full_path[i].first, full_path[i].second));
    }
    if (full_path.back() != std::pair<int,int>(control_pts.back().first, control_pts.back().second)) {
        control_pts.push_back({full_path.back().first, full_path.back().second});
        control_heights.push_back(map.at(full_path.back().first, full_path.back().second));
    }

    if (control_pts.size() < 4) {
        for (const auto& p : full_path) {
            road_mask[p.second * map.width + p.first] = true;
        }
        return road_mask;
    }

    std::vector<SmoothPoint> smooth_path;
    
    for (size_t i = 0; i < control_pts.size() - 1; ++i) {
        auto p0 = (i == 0) ? control_pts[0] : control_pts[i - 1];
        auto p1 = control_pts[i];
        auto p2 = control_pts[i + 1];
        auto p3 = (i + 2 < control_pts.size()) ? control_pts[i + 2] : p2;

        float h0 = (i == 0) ? control_heights[0] : control_heights[i - 1];
        float h1 = control_heights[i];
        float h2 = control_heights[i + 1];
        float h3 = (i + 2 < control_heights.size()) ? control_heights[i + 2] : h2;
        
        int steps = 10;
        for (int step = 0; step < steps; ++step) {
            float t = static_cast<float>(step) / steps;
            auto pt = catmullRom2D(p0, p1, p2, p3, t);
            float h = catmullRom1D(h0, h1, h2, h3, t);
            smooth_path.push_back({static_cast<int>(std::round(pt.first)), static_cast<int>(std::round(pt.second)), h});
        }
    }

    // Flatten terrain along road
    int r = params.road_radius;
    for (const auto& sp : smooth_path) {
        if (sp.x < 0 || sp.x >= map.width || sp.y < 0 || sp.y >= map.height) continue;
        
        for (int dy = -r; dy <= r; ++dy) {
            for (int dx = -r; dx <= r; ++dx) {
                int nx = sp.x + dx;
                int ny = sp.y + dy;
                
                if (nx >= 0 && nx < map.width && ny >= 0 && ny < map.height) {
                    float dist = std::sqrt(static_cast<float>(dx * dx + dy * dy));
                    if (dist <= r) {
                        road_mask[ny * map.width + nx] = true;
                        
                        float current_h = map.at(nx, ny);
                        float blend_factor = (1.0f - (dist / (r + 1.0f))) * params.flatten_amount;
                        map.at(nx, ny) = current_h * (1.0f - blend_factor) + sp.height * blend_factor;
                    }
                }
            }
        }
    }
    
    return road_mask;
}
