#pragma once

#include <memory>
#include <string>
#include <vector>

#include "core/IRecognizer.h"

struct RibParameters : public IParameters
{
    double lengthMm = 3.0;
};

struct RibInstance
{
    int index = 0;
    double lengthMm = 0.0;
    std::vector<int> topFaceIds;
};

class RibResult : public IFeatureResult
{
public:
    RibResult(double lengthUsedMm, std::vector<RibInstance> instances)
        : iLengthUsedMm(lengthUsedMm), iInstances(std::move(instances))
    {
    }

    std::string describe() const override;
    double lengthUsedMm() const { return iLengthUsedMm; }
    const std::vector<RibInstance> &instances() const { return iInstances; }

private:
    double iLengthUsedMm;
    std::vector<RibInstance> iInstances;
};

class RibRecognizer : public IRecognizer
{
public:
    std::string name() const override { return "rib"; }
    // AAG is covered transitively through fillet; declaring it again would
    // be redundant (SPEC section 5).
    std::vector<std::string> dependencies() const override { return {"fillet"}; }
    bool isUserVisible() const override { return true; }
    std::shared_ptr<const IParameters> defaultParameters() const override
    {
        return std::make_shared<RibParameters>();
    }
    RecognitionOutcome recognize(const RecognitionContext &context) override;
};
