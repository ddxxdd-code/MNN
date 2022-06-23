#include "fusion_5_2.hpp"
#include <MNN/expr/ExprCreator.hpp>

using namespace MNN::Express;

VARP fusion_5_2(int numClass) {
    auto x = _Input({1, 1024, 40, 40});
    x = _Conv(0.0f, 0.0f, x, {1024, 1024}, {3, 3}, SAME);
    return x;
}