#pragma once

#include <chrono>
#include <string>
#include <utility>
#include <vector>

#include "core/FeatureId.h"

// What happened to one feature during a run.
struct FeatureRunReport
{
    enum class Outcome
    {
        Recognized,
        Skipped,
        Failed
    };

    explicit FeatureRunReport(FeatureId id) : feature(std::move(id)) {}

    FeatureId feature;
    Outcome outcome = Outcome::Recognized;
    bool wasAutoAdded = false; // upstream added by the engine, not requested
    std::string detail;        // skip reason / error message
    std::chrono::milliseconds elapsed{0};
};

// Returned by every recognize() call: what ran, what was skipped and why,
// how long each took (SPEC section 3.3).
struct RecognitionReport
{
    std::string globalError; // set when the run could not start at all
    std::vector<FeatureRunReport> entries;

    bool allSucceeded() const
    {
        if (!globalError.empty())
            return false;
        for (const auto &entry : entries)
            if (entry.outcome != FeatureRunReport::Outcome::Recognized)
                return false;
        return true;
    }
};
