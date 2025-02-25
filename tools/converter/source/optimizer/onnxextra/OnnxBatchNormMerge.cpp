//
//  OnnxBatchNormMerge.cpp
//  MNNConverter
//
//  Created by MNN on 2019/10/16.
//  Copyright © 2018, Alibaba Group Holding Limited
//

#include <math.h>
#include "MNN_generated.h"
#include "OnnxExtraManager.hpp"
namespace MNN {
namespace Express {
class OnnxBatchNormTransform : public OnnxExtraManager::Transform {
    virtual EXPRP onExecute(EXPRP expr) const override {
        auto inputs = expr->inputs();

        MNN_THROW_CHECK(inputs.size() == 5, "BatchNorm should have 5 inputs");

        int channels  = 1;
        float epsilon = 1e-10;

        auto bnOp       = expr->get();
        auto extraParam = bnOp->main_as_Extra();
        int size        = 0;
        if (nullptr != extraParam->attr()) {
            size = extraParam->attr()->size();
            for (int i = 0; i < size; ++i) {
                auto attr       = extraParam->attr()->GetAs<Attribute>(i);
                const auto& key = attr->key()->str();
                if (key == "epsilon") {
                    epsilon = attr->f();
                }
            }
        }

        auto gamma    = inputs[1];
        auto beta     = inputs[2];
        auto mean     = inputs[3];
        auto variance = inputs[4];

        MNN_THROW_CHECK(gamma->getInfo() != nullptr, "BatchNorm second input should be Constant!");
        MNN_THROW_CHECK(beta->getInfo() != nullptr, "BatchNorm second input should be Constant!");
        MNN_THROW_CHECK(mean->getInfo() != nullptr, "BatchNorm second input should be Constant!");
        MNN_THROW_CHECK(variance->getInfo() != nullptr, "BatchNorm second input should be Constant!");
        auto gammaSize    = gamma->getInfo()->size;
        auto betaSize     = beta->getInfo()->size;
        auto meanSize     = mean->getInfo()->size;
        auto varianceSize = variance->getInfo()->size;

        // find the max value(incase broadcast mode)
        channels = gammaSize > betaSize ? gammaSize : betaSize;
        channels = channels > meanSize ? channels : meanSize;
        channels = channels > varianceSize ? channels : varianceSize;

        std::unique_ptr<MNN::BatchNormT> batchnorm(new MNN::BatchNormT);
        batchnorm->slopeData.resize(channels);
        batchnorm->biasData.resize(channels);
        batchnorm->meanData.resize(channels);
        batchnorm->varData.resize(channels);
        batchnorm->channels = channels;

        // TODO check data length, then support broadcast mode
        auto gammaDataPtr = gamma->readMap<float>();
        MNN_THROW_CHECK(gammaDataPtr != nullptr, "BatchNorm's gamma not valid!");
        memcpy(batchnorm->slopeData.data(), gammaDataPtr, gamma->getInfo()->size * sizeof(float));

        auto betaDataPtr = beta->readMap<float>();
        MNN_THROW_CHECK(betaDataPtr != nullptr, "BatchNorm's beta not valid!");
        memcpy(batchnorm->biasData.data(), betaDataPtr, beta->getInfo()->size * sizeof(float));
        auto meanDataPtr = mean->readMap<float>();
        MNN_THROW_CHECK(meanDataPtr != nullptr, "BatchNorm's mean not valid!");
        memcpy(batchnorm->meanData.data(), meanDataPtr, mean->getInfo()->size * sizeof(float));
        auto varPtr = variance->readMap<float>();
        MNN_THROW_CHECK(varPtr != nullptr, "BatchNorm's var not valid!");
        for (int i = 0; i < channels; ++i) {
            batchnorm->varData[i] = varPtr[i];
        }

        std::unique_ptr<OpT> mnnBnOp(new OpT);
        mnnBnOp->name      = expr->name();
        mnnBnOp->type      = OpType_BatchNorm;
        mnnBnOp->main.type = OpParameter_BatchNorm;
        {
            auto bnParam        = new MNN::BatchNormT;
            mnnBnOp->main.value = bnParam;
            bnParam->channels   = batchnorm->channels;
            bnParam->slopeData.resize(batchnorm->channels);
            bnParam->biasData.resize(batchnorm->channels);
            bnParam->meanData.resize(batchnorm->channels);
            bnParam->varData.resize(batchnorm->channels);
            const float* slopeDataPtr = batchnorm->slopeData.data();
            const float* biasDataPtr  = batchnorm->biasData.data();
            const float* meanDataPtr  = batchnorm->meanData.data();
            const float* varDataPtr   = batchnorm->varData.data();

            for (int i = 0; i < batchnorm->channels; i++) {
                bnParam->slopeData[i] = slopeDataPtr[i];
                bnParam->biasData[i]  = biasDataPtr[i];
                bnParam->meanData[i]  = meanDataPtr[i];
                bnParam->varData[i]   = varDataPtr[i];
            }
            bnParam->epsilon = epsilon;
        }
        // create merged op
        auto newExpr = Expr::create(mnnBnOp.get(), {inputs[0]});
        newExpr->setName(expr->name());
        auto res = Variable::create(newExpr);
        return res->expr().first;
    }
};

static VARP _OnnxReshape(VARP x, VARP shape) {
    std::unique_ptr<OpT> reshape(new OpT);
    reshape->type = OpType_Reshape;
    reshape->main.type = OpParameter_Reshape;
    reshape->main.value = new ReshapeT;
    reshape->main.AsReshape()->dimType = MNN_DATA_FORMAT_NCHW;
    return (Variable::create(Expr::create(reshape.get(), {x, shape})));
}

class OnnxInstanceNormalTransform : public OnnxExtraManager::Transform {
    virtual EXPRP onExecute(EXPRP expr) const override {
        auto inputs = expr->inputs();

        MNN_THROW_CHECK(inputs.size() == 3, "InstanceNormal should have 3 inputs");
        auto input = inputs[0];

        int channels  = 1;
        float epsilon = 1e-10;

        auto bnOp       = expr->get();
        auto extraParam = bnOp->main_as_Extra();
        int size        = 0;
        if (nullptr != extraParam->attr()) {
            size = extraParam->attr()->size();
            for (int i = 0; i < size; ++i) {
                auto attr       = extraParam->attr()->GetAs<Attribute>(i);
                const auto& key = attr->key()->str();
                if (key == "epsilon") {
                    epsilon = attr->f();
                }
            }
        }

        auto compatShape = _Concat({_Shape(inputs[1], true), _Fill(_Unsqueeze(_Size(_Shape(input, true)) - _Scalar<int>(2), {0}), _Scalar<int>(1))}, 0);
        auto scale      = _OnnxReshape(inputs[1], compatShape);
        auto bias       = _OnnxReshape(inputs[2], compatShape);
        auto epsilonVar = _Scalar<float>(epsilon);
        auto mean       = _ReduceMean(input, {2, 3}, true);
        auto temp       = input - mean;
        temp            = temp * temp;
        auto var        = _ReduceMean(temp, {2, 3}, true);
        auto varRev     = _Rsqrt(var + epsilonVar);
        auto alpha      = scale * varRev;
        auto beta       = bias - alpha * mean;
        auto res        = input * alpha + beta;
        res->setName(expr->name());
        return res->expr().first;
    }
};

class OnnxMeanVarianceNormTransform : public OnnxExtraManager::Transform {
    virtual EXPRP onExecute(EXPRP expr) const override {
        std::vector<int> axes {0, 2, 3};
        auto attrs = expr->get()->main_as_Extra()->attr();
        if (attrs != nullptr) {
            for (const auto& attr : *attrs) {
                if (attr->key()->str() == "axes") {
                    axes.clear();
                    for (int i = 0; i < attr->list()->i()->size(); ++i) {
                        axes.push_back(attr->list()->i()->Get(i));
                    }
                }
            }
        }
        auto input = expr->inputs()[0];
        auto mean = _ReduceMean(input, axes, true);
        auto temp = input - mean;
        auto var = _ReduceMean(temp * temp, axes, true);
        auto res = temp * _Rsqrt(var);
        res->setName(expr->name());
        return res->expr().first;
    }
};

class OnnxLpNormTransform : public OnnxExtraManager::Transform {
    virtual EXPRP onExecute(EXPRP expr) const override {
        auto input = expr->inputs()[0];
        int p = 2, axis = -1;
        auto attrs = expr->get()->main_as_Extra()->attr();
        if (attrs != nullptr) {
            for (const auto& attr : *attrs) {
                auto attrName = attr->key()->str();
                if (attrName == "axis") {
                    axis = attr->i();
                } else if (attrName == "p") {
                    p = attr->i();
                }
            }
        }
        if (p != 1 && p != 2) {
            MNN_ERROR("Onnx's LpNormalization only support attr p is 1 or 2");
            return nullptr;
        }
        VARP res;
        if (p == 1) {
            res = input / _ReduceSumMutable(_Abs(input), _Scalar<int>(axis), true);
        } else {
            res = input * _Rsqrt(_ReduceSumMutable(input * input, _Scalar<int>(axis), true));
        }
        res->setName(expr->name());
        return res->expr().first;
    }
};

static auto gRegister = []() {
    OnnxExtraManager::get()->insert("BatchNormalization",
                                    std::shared_ptr<OnnxExtraManager::Transform>(new OnnxBatchNormTransform));
    OnnxExtraManager::get()->insert("InstanceNormalization",
                                    std::shared_ptr<OnnxExtraManager::Transform>(new OnnxInstanceNormalTransform));
    OnnxExtraManager::get()->insert("MeanVarianceNormalization",
                                    std::shared_ptr<OnnxExtraManager::Transform>(new OnnxMeanVarianceNormTransform));
    OnnxExtraManager::get()->insert("LpNormalization",
                                    std::shared_ptr<OnnxExtraManager::Transform>(new OnnxLpNormTransform));
    return true;
}();

} // namespace Express
} // namespace MNN
