#include "Filters.h"
#include <vector>

void Filters::fastBoxBlur(Heightmap& map, int radius) {
    if (radius <= 0) return;

    int width = map.width;
    int height = map.height;
    std::vector<float> temp(width * height);

    // Horizontal pass
    for (int y = 0; y < height; ++y) {
        float sum = 0.0f;
        int count = 0;

        // Init window
        for (int i = -radius; i <= radius; ++i) {
            int px = std::max(0, std::min(width - 1, i));
            sum += map.data[y * width + px];
            count++;
        }

        for (int x = 0; x < width; ++x) {
            temp[y * width + x] = sum / count;

            int left = std::max(0, x - radius);
            int right = std::min(width - 1, x + radius + 1);

            sum -= map.data[y * width + left];
            sum += map.data[y * width + right];
        }
    }

    // Vertical pass
    for (int x = 0; x < width; ++x) {
        float sum = 0.0f;
        int count = 0;

        // Init window
        for (int i = -radius; i <= radius; ++i) {
            int py = std::max(0, std::min(height - 1, i));
            sum += temp[py * width + x];
            count++;
        }

        for (int y = 0; y < height; ++y) {
            map.data[y * width + x] = sum / count;

            int top = std::max(0, y - radius);
            int bottom = std::min(height - 1, y + radius + 1);

            sum -= temp[top * width + x];
            sum += temp[bottom * width + x];
        }
    }
}
