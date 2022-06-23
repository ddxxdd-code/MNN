#include "vgg_1_2.hpp"
#include <MNN/expr/ExprCreator.hpp>

using namespace MNN::Express;

VARP vgg_1_2(int numClass) {
    auto x = _Input({1, 64, 224, 224});
    x = _Conv(0.0f, 0.0f, x, {64, 64}, {3, 3}, SAME);
    return x;
}