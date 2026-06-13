#include "HydraulicErosion.h"
#include <glm/glm.hpp>
#include <random>
#include <algorithm>
#include <cmath>
#include <omp.h>
#include <iostream>

struct Gradient {
    float height;
    float gradientX;
    float gradientY;
};

static Gradient calculateGradientAndHeight(const Heightmap& map, float posX, float posY) {
    int coordX = static_cast<int>(posX);
    int coordY = static_cast<int>(posY);
    
    float x = posX - coordX;
    float y = posY - coordY;

    int cx0 = std::clamp(coordX, 0, map.width - 1);
    int cx1 = std::clamp(coordX + 1, 0, map.width - 1);
    int cy0 = std::clamp(coordY, 0, map.height - 1);
    int cy1 = std::clamp(coordY + 1, 0, map.height - 1);

    float heightNW = map.at(cx0, cy0);
    float heightNE = map.at(cx1, cy0);
    float heightSW = map.at(cx0, cy1);
    float heightSE = map.at(cx1, cy1);

    float gradientX = (heightNE - heightNW) * (1.0f - y) + (heightSE - heightSW) * y;
    float gradientY = (heightSW - heightNW) * (1.0f - x) + (heightSE - heightNE) * x;

    float height = heightNW * (1.0f - x) * (1.0f - y) + 
                   heightNE * x * (1.0f - y) + 
                   heightSW * (1.0f - x) * y + 
                   heightSE * x * y;

    return {height, gradientX, gradientY};
}

void HydraulicErosion::erode(Heightmap& map, const ErosionParams& params, int seed) {
    // Hydraulic droplets first

    #pragma omp parallel
    {
        std::mt19937 rng(seed + omp_get_thread_num());
        std::uniform_real_distribution<float> randX(0.0f, static_cast<float>(map.width - 2));
        std::uniform_real_distribution<float> randY(0.0f, static_cast<float>(map.height - 2));

        #pragma omp for
        for (int i = 0; i < params.num_droplets; ++i) {
            float posX = randX(rng);
            float posY = randY(rng);
            float dirX = 0.0f;
            float dirY = 0.0f;
            float speed = params.initial_velocity;
            float water = params.initial_water;
            float sediment = 0.0f;

            for (int step = 0; step < params.max_steps; ++step) {
                int nodeX = static_cast<int>(posX);
                int nodeY = static_cast<int>(posY);
                
                float cellOffsetX = posX - nodeX;
                float cellOffsetY = posY - nodeY;

                Gradient grad = calculateGradientAndHeight(map, posX, posY);

                dirX = (dirX * params.inertia) - (grad.gradientX * (1.0f - params.inertia));
                dirY = (dirY * params.inertia) - (grad.gradientY * (1.0f - params.inertia));

                float len = std::sqrt(dirX * dirX + dirY * dirY);
                if (len != 0) {
                    dirX /= len;
                    dirY /= len;
                }

                posX += dirX;
                posY += dirY;

                if ((dirX == 0 && dirY == 0) || posX < 0 || posX >= map.width - 1 || posY < 0 || posY >= map.height - 1) {
                    break;
                }

                float newHeight = calculateGradientAndHeight(map, posX, posY).height;
                float deltaHeight = newHeight - grad.height;

                // Capacidad Relativa: La fuerza del agua es exponencial a la inclinación de la montaña.
                float slopeFactor = (std::max)(-deltaHeight, params.min_sediment_capacity);
                // By multiplying slopeFactor * (1.0f + slopeFactor * 50.0f), 
                // flat terrain erodes very little, while steep cliffs erode massively.
                float relativeCapacity = slopeFactor * (1.0f + slopeFactor * 50.0f);
                float capacity = relativeCapacity * speed * water * params.sediment_capacity;

                if (sediment > capacity || deltaHeight > 0) {
                    float amountToDeposit = (deltaHeight > 0) ? (std::min)(deltaHeight, sediment) : (sediment - capacity) * params.deposit_speed;
                    sediment -= amountToDeposit;

                    #pragma omp atomic
                    map.data[nodeY * map.width + nodeX] += amountToDeposit * (1.0f - cellOffsetX) * (1.0f - cellOffsetY);
                    #pragma omp atomic
                    map.data[nodeY * map.width + (nodeX + 1)] += amountToDeposit * cellOffsetX * (1.0f - cellOffsetY);
                    #pragma omp atomic
                    map.data[(nodeY + 1) * map.width + nodeX] += amountToDeposit * (1.0f - cellOffsetX) * cellOffsetY;
                    #pragma omp atomic
                    map.data[(nodeY + 1) * map.width + (nodeX + 1)] += amountToDeposit * cellOffsetX * cellOffsetY;
                } else {
                    float amountToErode = (std::min)((capacity - sediment) * params.erode_speed, -deltaHeight);
                    
                    #pragma omp atomic
                    map.data[nodeY * map.width + nodeX] -= amountToErode * (1.0f - cellOffsetX) * (1.0f - cellOffsetY);
                    #pragma omp atomic
                    map.data[nodeY * map.width + (nodeX + 1)] -= amountToErode * cellOffsetX * (1.0f - cellOffsetY);
                    #pragma omp atomic
                    map.data[(nodeY + 1) * map.width + nodeX] -= amountToErode * (1.0f - cellOffsetX) * cellOffsetY;
                    #pragma omp atomic
                    map.data[(nodeY + 1) * map.width + (nodeX + 1)] -= amountToErode * cellOffsetX * cellOffsetY;

                    sediment += amountToErode;
                }

                speed = std::sqrt((std::max)(0.0f, speed * speed + deltaHeight * params.gravity));
                water *= (1.0f - params.evaporate_speed);
            }
        }
    }

    // Thermal weathering (Slippage) post-pass to smooth out hydraulic noise
    if (params.thermal_iterations > 0) {
        std::vector<float> next_data = map.data;
        for (int i = 0; i < params.thermal_iterations; ++i) {
            #pragma omp parallel for
            for (int y = 1; y < map.height - 1; ++y) {
                for (int x = 1; x < map.width - 1; ++x) {
                    float h = map.at(x, y);
                    float max_diff = 0.0f;
                    int max_dx = 0, max_dy = 0;
                    
                    // Find steepest neighbor
                    for (int dy = -1; dy <= 1; ++dy) {
                        for (int dx = -1; dx <= 1; ++dx) {
                            if (dx == 0 && dy == 0) continue;
                            float diff = h - map.at(x + dx, y + dy);
                            // Adjust for diagonal distance
                            if (dx != 0 && dy != 0) diff /= 1.414f;
                            if (diff > max_diff) {
                                max_diff = diff;
                                max_dx = dx;
                                max_dy = dy;
                            }
                        }
                    }
                    
                    if (max_diff > params.thermal_slope_threshold) {
                        float transfer = max_diff * 0.5f; // move half the difference
                        #pragma omp atomic
                        next_data[y * map.width + x] -= transfer;
                        #pragma omp atomic
                        next_data[(y + max_dy) * map.width + (x + max_dx)] += transfer;
                    }
                }
            }
            map.data = next_data;
        }
    }
}
