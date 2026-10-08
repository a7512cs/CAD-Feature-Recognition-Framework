#pragma once

#include <memory>
#include <string>
#include <vector>

#include "core/IRecognizer.h"

struct HoleParameters : public IParameters
{
    double radiusMm = 5.0;
};

struct HoleInstance
{
    int index = 0;
    double radiusMm = 0.0;
    std::vector<int> faceIds;
};

class HoleResult : public IFeatureResult
{
public:
    HoleResult(double radiusUsedMm, std::vector<HoleInstance> instances)
        : iRadiusUsedMm(radiusUsedMm), iInstances(std::move(instances))
    {
    }

    std::string describe() const override;
    double radiusUsedMm() const { return iRadiusUsedMm; }
    const std::vector<HoleInstance> &instances() const { return iInstances; }

private:
    double iRadiusUsedMm;
    std::vector<HoleInstance> iInstances;
};

class HoleRecognizer : public IRecognizer
{
public:
    std::string name() const override { return "hole"; }
    // AAG is covered transitively through fillet (SPEC section 5).
    std::vector<std::string> dependencies() const override { return {"fillet"}; }
    bool isUserVisible() const override { return true; }
    std::shared_ptr<const IParameters> defaultParameters() const override
    {
        return std::make_shared<HoleParameters>();
    }
    RecognitionOutcome recognize(const RecognitionContext &context) override;
};
