#pragma once

#include <cstddef>
#include <functional>
#include <string>

// Identity of a registered feature. Issued by RecognitionEngine at
// registration time; the core compares ids, never strings (ADR-0003).
class FeatureId
{
public:
    const std::string &name() const { return iName; }
    std::size_t index() const { return iIndex; }

    bool operator==(const FeatureId &other) const { return iIndex == other.iIndex; }
    bool operator!=(const FeatureId &other) const { return !(*this == other); }

private:
    friend class RecognitionEngine;
    FeatureId(std::size_t index, std::string name) : iIndex(index), iName(std::move(name)) {}

    std::size_t iIndex;
    std::string iName;
};

namespace std
{
template <> struct hash<FeatureId>
{
    std::size_t operator()(const FeatureId &id) const noexcept { return id.index(); }
};
} // namespace std
