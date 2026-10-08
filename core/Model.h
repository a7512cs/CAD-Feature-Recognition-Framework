#pragma once

#include <memory>
#include <unordered_map>

#include "core/BRep.h"
#include "core/FeatureId.h"
#include "core/FeatureStatus.h"
#include "core/GeometryImport.h"
#include "core/IFeatureResult.h"

// One recognition target: an imported BRep plus every feature result on it.
class Model
{
public:
    Model() = default;
    Model(const Model &) = delete;
    Model &operator=(const Model &) = delete;
    Model(Model &&) = default;
    Model &operator=(Model &&) = default;
    ~Model() = default;

    // Adopt (re)imported geometry. Clears every result: ids are reassigned,
    // so a kept result would point at the wrong faces (SPEC section 3.6).
    void acceptImport(BRep brep, IdCorrespondence ids);

    bool hasGeometry() const { return iHasGeometry; }
    const BRep &brep() const { return iBRep; }
    const IdCorrespondence &hostIds() const { return iHostIds; }

    FeatureStatus status(const FeatureId &feature) const;

    // Readable even when Outdated (SPEC section 3.5); null when NeverRecognized.
    const IFeatureResult *result(const FeatureId &feature) const;

    // Called by the engine.
    void storeResult(const FeatureId &feature, std::unique_ptr<IFeatureResult> result); // -> Valid
    void markOutdated(const FeatureId &feature); // no-op unless a result exists

private:
    struct Entry
    {
        std::unique_ptr<IFeatureResult> result;
        FeatureStatus status = FeatureStatus::NeverRecognized;
    };

    bool iHasGeometry = false;
    BRep iBRep;
    IdCorrespondence iHostIds;
    std::unordered_map<FeatureId, Entry> iEntries;
};
