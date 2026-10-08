#pragma once

#include <string>

// Result of one recognition. Callers that need typed data dynamic_cast to
// the concrete type; describe() serves logs and debug dumps, so a new
// feature prints itself without touching the core (see ADR-0003).
class IFeatureResult
{
public:
    virtual ~IFeatureResult() = default;
    virtual std::string describe() const = 0;
};
