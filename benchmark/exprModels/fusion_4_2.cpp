#include "fusion_4_2.hpp"
#include <MNN/expr/ExprCreator.hpp>

using namespace MNN::Express;

VARP fusion_4_2(int numClass) {
    auto x = _Input({1, 512, 80, 80});
    x = _Conv(0.0f, 0.0f, x, {512, 512}, {3, 3}, SAME);
    return x;
}