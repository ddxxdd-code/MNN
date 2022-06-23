#include "vgg_3_2.hpp"
#include <MNN/expr/ExprCreator.hpp>

using namespace MNN::Express;

VARP vgg_3_2(int numClass) {
    auto x = _Input({1, 256, 56, 56});
    x = _Conv(0.0f, 0.0f, x, {256, 256}, {3, 3}, SAME);
    return x;
}