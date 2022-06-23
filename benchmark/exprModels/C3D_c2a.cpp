#include "C3D_c2a.hpp"
#include <MNN/expr/Expr.hpp>
#include <MNN/expr/ExprCreator.hpp>
#include <MNN/expr/Optimizer.hpp>
#include <numeric>
#include <vector>

using namespace MNN;
using namespace MNN::Express;

VARP C3DC2A(int numClass) {
    int batch = 32;
    int inc = 64;
    int ouc = 128;
    int d = 16;
    int w = 56;
    int h = 56;
    std::vector<float> weightData, biasData;
    for (int i = 0; i < batch * inc * ouc / 8 * d * h * w; i++) {
        weightData.push_back(rand() % 255 / 2550.f);
    }
    for (int i = 0; i < ouc; i++) {
        biasData.push_back(rand() % 255 / 255.f);
    }
    std::vector<float> inputData, outputData;
    for (int i = 0; i < batch * inc * d * h * w; ++i) {
        inputData.push_back(rand() % 255 / 255.f);
    }
    auto x = _Input();
    // auto x = _Conv3D(inputData, weightData, biasData, {inc, ouc}, {3, 3, 3}, PadMode_CAFFE, {1, 1, 1}, {1, 1, 1},
    //                           0, 8);

    return x;
}