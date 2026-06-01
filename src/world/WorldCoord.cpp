#include "world/WorldCoord.h"

#include <cmath>

namespace WorldCoord {

int floorDiv(int a, int b) {
    int q = a / b;
    int r = a % b;
    if (r != 0 && ((r > 0) != (b > 0))) {
        --q;
    }
    return q;
}

int positiveMod(int a, int b) {
    int m = a % b;
    if (m < 0) {
        m += (b < 0 ? -b : b);
    }
    return m;
}

int worldToBlock(float v) {
    return static_cast<int>(std::floor(v + 0.5f));
}

} // namespace WorldCoord
