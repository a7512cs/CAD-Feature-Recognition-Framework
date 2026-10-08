#include "core/Model.h"

void Model::acceptImport(BRep brep, IdCorrespondence ids)
{
    iBRep = std::move(brep);
    iHostIds = std::move(ids);
    iHasGeometry = true;
    iEntries.clear(); // every feature back to NeverRecognized (SPEC section 3.6)
}

FeatureStatus Model::status(const FeatureId &feature) const
{
    auto it = iEntries.find(feature);
    return it == iEntries.end() ? FeatureStatus::NeverRecognized : it->second.status;
}

const IFeatureResult *Model::result(const FeatureId &feature) const
{
    auto it = iEntries.find(feature);
    return it == iEntries.end() ? nullptr : it->second.result.get();
}

void Model::storeResult(const FeatureId &feature, std::unique_ptr<IFeatureResult> result)
{
    auto &entry = iEntries[feature];
    entry.result = std::move(result);
    entry.status = FeatureStatus::Valid;
}

void Model::markOutdated(const FeatureId &feature)
{
    auto it = iEntries.find(feature);
    if (it == iEntries.end() || !it->second.result)
        return;
    it->second.status = FeatureStatus::Outdated;
}
