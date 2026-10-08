#pragma once

#include <memory>
#include <string>
#include <vector>

#include "core/IRecognizer.h"

// AAG = face adjacency graph. Internal feature: users never see it, but
// every other recognizer depends on it, directly or transitively.

struct AAGParameters : public IParameters
{
    double angleThresholdDeg = 30.0;
};

// Derived conclusion (convexity) lives here, not in BRep (SPEC section 3.2).
struct AAGAdjacency
{
    int faceA = 0;
    int faceB = 0;
    bool isConvex = false;
};

class AAGResult : public IFeatureResult
{
public:
    AAGResult(double angleUsedDeg, std::vector<AAGAdjacency> adjacencies)
        : iAngleUsedDeg(angleUsedDeg), iAdjacencies(std::move(adjacencies))
    {
    }

    std::string describe() const override;
    double angleUsedDeg() const { return iAngleUsedDeg; }
    const std::vector<AAGAdjacency> &adjacencies() const { return iAdjacencies; }

private:
    double iAngleUsedDeg;
    std::vector<AAGAdjacency> iAdjacencies;
};

class AAGRecognizer : public IRecognizer
{
public:
    std::string name() const override { return "aag"; }
    std::vector<std::string> dependencies() const override { return {}; }
    bool isUserVisible() const override { return false; }
    std::shared_ptr<const IParameters> defaultParameters() const override
    {
        return std::make_shared<AAGParameters>();
    }
    RecognitionOutcome recognize(const RecognitionContext &context) override;
};
