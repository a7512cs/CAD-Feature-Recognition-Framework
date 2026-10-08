#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "core/BRep.h"
#include "core/IFeatureResult.h"
#include "core/IParameters.h"

// Everything a recognizer may see while running: the geometry, its own
// parameters, and the results of its declared dependencies.
class RecognitionContext
{
public:
    RecognitionContext(const BRep &brep, const IParameters &parameters,
                       std::unordered_map<std::string, const IFeatureResult *> dependencyResults)
        : iBRep(brep), iParameters(parameters), iDependencyResults(std::move(dependencyResults))
    {
    }

    const BRep &brep() const { return iBRep; }
    const IParameters &parameters() const { return iParameters; }

    // Result of a declared dependency, by feature name. Null when absent.
    const IFeatureResult *dependencyResult(const std::string &featureName) const
    {
        auto it = iDependencyResults.find(featureName);
        return it == iDependencyResults.end() ? nullptr : it->second;
    }

private:
    const BRep &iBRep;
    const IParameters &iParameters;
    std::unordered_map<std::string, const IFeatureResult *> iDependencyResults;
};

// Errors travel as values, not exceptions (SPEC section 3.7).
struct RecognitionOutcome
{
    std::unique_ptr<IFeatureResult> result; // null on failure
    std::string error;                      // set on failure

    static RecognitionOutcome success(std::unique_ptr<IFeatureResult> recognized)
    {
        RecognitionOutcome outcome;
        outcome.result = std::move(recognized);
        return outcome;
    }

    static RecognitionOutcome failure(std::string message)
    {
        RecognitionOutcome outcome;
        outcome.error = std::move(message);
        return outcome;
    }
};

// One feature's recognition implementation. Declares its own identity,
// dependencies and defaults; the core never knows concrete features
// (ADR-0003).
class IRecognizer
{
public:
    virtual ~IRecognizer() = default;
    virtual std::string name() const = 0;
    virtual std::vector<std::string> dependencies() const = 0; // direct upstream names
    virtual bool isUserVisible() const = 0; // false = internal feature (e.g. AAG)
    virtual std::shared_ptr<const IParameters> defaultParameters() const = 0;
    virtual RecognitionOutcome recognize(const RecognitionContext &context) = 0;
};
