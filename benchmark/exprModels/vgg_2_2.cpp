#include "vgg_2_2.hpp"
#include <MNN/expr/ExprCreator.hpp>

using namespace MNN::Express;

VARP vgg_2_2(int numClass) {
    auto x = _Input({1, 128, 112, 112});
    x = _Conv(0.0f, 0.0f, x, {128, 128}, {3, 3}, SAME);
    return x;
}