#pragma once

#include <memory>

#include "core/FeatureId.h"
#include "core/IParameters.h"

// Host-implemented, per-model persistence of "parameters last used in a
// successful recognition" (SPEC section 3.4). The core reads it when
// auto-recomputing an upstream and writes it after every success.
// Reimport must NOT clear it (SPEC section 3.6).
class IParameterStore
{
public:
    virtual ~IParameterStore() = default;

    // Null when the feature never ran successfully.
    virtual std::shared_ptr<const IParameters>
    lastUsedParameters(const FeatureId &feature) const = 0;
    virtual void saveUsedParameters(const FeatureId &feature,
                                    std::shared_ptr<const IParameters> parameters) = 0;
};
