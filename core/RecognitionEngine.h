#pragma once

#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "core/DependencyGraph.h"
#include "core/FeatureId.h"
#include "core/IParameterStore.h"
#include "core/IRecognitionListener.h"
#include "core/IRecognizer.h"
#include "core/Model.h"
#include "core/RecognitionReport.h"

struct RegistrationResult
{
    std::optional<FeatureId> id; // empty on error
    std::string error;
};

struct FinalizeResult
{
    bool ok = false;
    std::string error;
};

// What the caller wants recognized, plus explicit parameters for this run.
// Parameter resolution order: request -> store (last used) -> defaults
// (SPEC section 3.4).
struct RecognitionRequest
{
    std::vector<FeatureId> targets;
    std::unordered_map<FeatureId, std::shared_ptr<const IParameters>> parameters;
};

// The framework core. Owns the registry and the dependency graph; expanding
// upstream, ordering, running and outdating are all inferences over that
// graph — the core knows no concrete feature (ADR-0003).
class RecognitionEngine
{
public:
    RecognitionEngine(IParameterStore &parameterStore, IRecognitionListener &listener);

    RegistrationResult registerRecognizer(std::unique_ptr<IRecognizer> recognizer);
    FinalizeResult finalizeRegistration(); // resolves names, rejects unknown deps and cycles

    std::optional<FeatureId> featureByName(const std::string &name) const;
    const std::vector<FeatureId> &allFeatures() const { return iIds; }
    std::vector<FeatureId> directUpstream(const FeatureId &feature) const;
    bool isUserVisible(const FeatureId &feature) const;

    RecognitionReport recognize(Model &model, const RecognitionRequest &request);

private:
    std::vector<FeatureId> expandTargets(const Model &model,
                                         const std::vector<FeatureId> &targets) const;
    std::shared_ptr<const IParameters> resolveParameters(const FeatureId &feature,
                                                         const RecognitionRequest &request) const;
    FeatureRunReport runOne(Model &model, const FeatureId &feature, const IParameters &parameters);
    void propagateOutdated(Model &model, const FeatureId &cause);

    IParameterStore &iParameterStore;
    IRecognitionListener &iListener;
    std::vector<std::unique_ptr<IRecognizer>> iRecognizers;
    std::vector<FeatureId> iIds; // index == FeatureId::index()
    std::unordered_map<std::string, std::size_t> iIndexByName;
    DependencyGraph iGraph;
    bool iFinalized = false;
};
