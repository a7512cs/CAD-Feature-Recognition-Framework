#pragma once

#include <chrono>
#include <string>

#include "core/FeatureId.h"

// Host-implemented sink for live progress, per-feature timing and outdated
// notifications. One interface covers both logging and progress, since the
// implementer is always the same thing.
class IRecognitionListener
{
public:
    virtual ~IRecognitionListener() = default;
    virtual void onRecognitionStarted(const FeatureId &feature) = 0;
    virtual void onRecognitionSucceeded(const FeatureId &feature,
                                        std::chrono::milliseconds elapsed) = 0;
    virtual void onRecognitionFailed(const FeatureId &feature, const std::string &error) = 0;
    virtual void onFeatureMarkedOutdated(const FeatureId &affected, const FeatureId &cause) = 0;
};
