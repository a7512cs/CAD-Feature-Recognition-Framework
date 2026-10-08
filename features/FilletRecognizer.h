#pragma once

#include <memory>
#include <string>
#include <vector>

#include "core/IRecognizer.h"

struct FilletParameters : public IParameters
{
    double radiusMm = 1.0;
};

struct FilletInstance
{
    int index = 0;
    double radiusMm = 0.0;
    std::vector<int> faceIds;
};

class FilletResult : public IFeatureResult
{
public:
    FilletResult(double radiusUsedMm, std::vector<FilletInstance> instances)
        : iRadiusUsedMm(radiusUsedMm), iInstances(std::move(instances))
    {
    }

    std::string describe() const override;
    double radiusUsedMm() const { return iRadiusUsedMm; }
    const std::vector<FilletInstance> &instances() const { return iInstances; }

private:
    double iRadiusUsedMm;
    std::vector<FilletInstance> iInstances;
};

class FilletRecognizer : public IRecognizer
{
public:
    std::string name() const override { return "fillet"; }
    std::vector<std::string> dependencies() const override { return {"aag"}; }
    bool isUserVisible() const override { return true; }
    std::shared_ptr<const IParameters> defaultParameters() const override
    {
        return std::make_shared<FilletParameters>();
    }
    RecognitionOutcome recognize(const RecognitionContext &context) override;
};
