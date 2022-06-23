#include "fusion_2_2.hpp"
#include <MNN/expr/ExprCreator.hpp>

using namespace MNN::Express;

VARP fusion_2_2(int numClass) {
    auto x = _Input({1, 128, 320, 320});
    x = _Conv(0.0f, 0.0f, x, {128, 128}, {3, 3}, SAME);
    return x;
}