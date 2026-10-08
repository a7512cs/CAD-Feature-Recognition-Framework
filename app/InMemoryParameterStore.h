#pragma once

#include <memory>
#include <unordered_map>

#include "core/IParameterStore.h"

// POC store: per-model in-memory map. A real host plugs its own preference
// system here instead. Held outside Model so reimport keeps it
// (SPEC section 3.6).
class InMemoryParameterStore : public IParameterStore
{
public:
    std::shared_ptr<const IParameters> lastUsedParameters(const FeatureId &feature) const override
    {
        auto it = iParameters.find(feature);
        return it == iParameters.end() ? nullptr : it->second;
    }

    void saveUsedParameters(const FeatureId &feature,
                            std::shared_ptr<const IParameters> parameters) override
    {
        iParameters[feature] = std::move(parameters);
    }

private:
    std::unordered_map<FeatureId, std::shared_ptr<const IParameters>> iParameters;
};
