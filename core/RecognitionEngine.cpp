#include "core/RecognitionEngine.h"

#include <unordered_set>

RecognitionEngine::RecognitionEngine(IParameterStore &parameterStore,
                                     IRecognitionListener &listener)
    : iParameterStore(parameterStore), iListener(listener)
{
}

RegistrationResult RecognitionEngine::registerRecognizer(std::unique_ptr<IRecognizer> recognizer)
{
    RegistrationResult result;
    if (!recognizer)
    {
        result.error = "recognizer is null";
        return result;
    }
    if (iFinalized)
    {
        result.error = "registration is already finalized";
        return result;
    }
    const std::string name = recognizer->name();
    if (name.empty())
    {
        result.error = "recognizer name is empty";
        return result;
    }
    if (iIndexByName.count(name))
    {
        result.error = "duplicate feature name '" + name + "'";
        return result;
    }

    const FeatureId id(iRecognizers.size(), name);
    iIndexByName.emplace(name, id.index());
    iIds.push_back(id);
    iRecognizers.push_back(std::move(recognizer));
    result.id = id;
    return result;
}

FinalizeResult RecognitionEngine::finalizeRegistration()
{
    FinalizeResult result;
    if (iFinalized)
    {
        result.error = "already finalized";
        return result;
    }

    DependencyGraph graph;
    for (const auto &id : iIds)
        graph.addNode(id);
    for (std::size_t i = 0; i < iRecognizers.size(); ++i)
    {
        for (const auto &dependencyName : iRecognizers[i]->dependencies())
        {
            auto it = iIndexByName.find(dependencyName);
            if (it == iIndexByName.end())
            {
                result.error =
                    "'" + iIds[i].name() + "' depends on unknown feature '" + dependencyName + "'";
                return result;
            }
            graph.addEdge(iIds[i], iIds[it->second]);
        }
    }
    if (!graph.isAcyclic())
    {
        result.error = "dependency graph has a cycle";
        return result;
    }

    iGraph = std::move(graph);
    iFinalized = true;
    result.ok = true;
    return result;
}

std::optional<FeatureId> RecognitionEngine::featureByName(const std::string &name) const
{
    auto it = iIndexByName.find(name);
    if (it == iIndexByName.end())
        return std::nullopt;
    return iIds[it->second];
}

std::vector<FeatureId> RecognitionEngine::directUpstream(const FeatureId &feature) const
{
    return iGraph.directUpstream(feature);
}

bool RecognitionEngine::isUserVisible(const FeatureId &feature) const
{
    return iRecognizers[feature.index()]->isUserVisible();
}

RecognitionReport RecognitionEngine::recognize(Model &model, const RecognitionRequest &request)
{
    RecognitionReport report;
    if (!iFinalized)
    {
        report.globalError = "registration is not finalized";
        return report;
    }
    if (!model.hasGeometry())
    {
        report.globalError = "no geometry imported";
        return report;
    }
    if (request.targets.empty())
    {
        report.globalError = "no targets requested";
        return report;
    }

    const std::unordered_set<FeatureId> requested(request.targets.begin(), request.targets.end());
    const auto order = expandTargets(model, request.targets);

    std::unordered_map<FeatureId, FeatureId> blockedBy; // feature -> failed upstream (root cause)
    for (const auto &feature : order)
    {
        if (auto it = blockedBy.find(feature); it != blockedBy.end())
        {
            FeatureRunReport entry{feature};
            entry.outcome = FeatureRunReport::Outcome::Skipped;
            entry.wasAutoAdded = requested.count(feature) == 0;
            entry.detail = "skipped: upstream '" + it->second.name() + "' failed";
            report.entries.push_back(std::move(entry));
            continue;
        }

        const auto parameters = resolveParameters(feature, request);
        auto entry = runOne(model, feature, *parameters);
        entry.wasAutoAdded = requested.count(feature) == 0;

        if (entry.outcome == FeatureRunReport::Outcome::Recognized)
        {
            // Success only (SPEC section 3.4): failed runs never become
            // the parameters auto-recompute falls back to.
            iParameterStore.saveUsedParameters(feature, parameters);
        }
        else
        {
            for (const auto &down : iGraph.downstreamClosure(feature))
                blockedBy.try_emplace(down, feature);
        }
        report.entries.push_back(std::move(entry));
    }
    return report;
}

std::vector<FeatureId> RecognitionEngine::expandTargets(const Model &model,
                                                        const std::vector<FeatureId> &targets) const
{
    std::unordered_set<FeatureId> todo(targets.begin(), targets.end());
    for (const auto &target : targets)
    {
        for (const auto &up : iGraph.upstreamClosure(target))
        {
            // Auto-recompute a missing or outdated upstream; a Valid one is
            // left alone (SPEC section 3.3).
            if (model.status(up) != FeatureStatus::Valid)
                todo.insert(up);
        }
    }
    return iGraph.topologicalOrder(std::vector<FeatureId>(todo.begin(), todo.end()));
}

std::shared_ptr<const IParameters>
RecognitionEngine::resolveParameters(const FeatureId &feature,
                                     const RecognitionRequest &request) const
{
    if (auto it = request.parameters.find(feature); it != request.parameters.end() && it->second)
        return it->second;
    if (auto lastUsed = iParameterStore.lastUsedParameters(feature))
        return lastUsed;
    return iRecognizers[feature.index()]->defaultParameters();
}

FeatureRunReport RecognitionEngine::runOne(Model &model, const FeatureId &feature,
                                           const IParameters &parameters)
{
    FeatureRunReport entry{feature};
    auto &recognizer = *iRecognizers[feature.index()];

    std::unordered_map<std::string, const IFeatureResult *> dependencyResults;
    for (const auto &up : iGraph.directUpstream(feature))
        dependencyResults[up.name()] = model.result(up);

    iListener.onRecognitionStarted(feature);
    const auto start = std::chrono::steady_clock::now();
    auto outcome = recognizer.recognize(
        RecognitionContext(model.brep(), parameters, std::move(dependencyResults)));
    entry.elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start);

    if (!outcome.result)
    {
        // Failure leaves the feature's state and old result untouched, and
        // does not outdate anything downstream (SPEC section 3.7).
        entry.outcome = FeatureRunReport::Outcome::Failed;
        entry.detail = outcome.error.empty() ? "recognition failed" : outcome.error;
        iListener.onRecognitionFailed(feature, entry.detail);
        return entry;
    }

    model.storeResult(feature, std::move(outcome.result));
    propagateOutdated(model, feature);
    iListener.onRecognitionSucceeded(feature, entry.elapsed);
    entry.outcome = FeatureRunReport::Outcome::Recognized;
    return entry;
}

void RecognitionEngine::propagateOutdated(Model &model, const FeatureId &cause)
{
    // Unconditional: no result comparison (ADR-0002). Only Valid flips to
    // Outdated (SPEC section 4).
    for (const auto &affected : iGraph.downstreamClosure(cause))
    {
        if (model.status(affected) != FeatureStatus::Valid)
            continue;
        model.markOutdated(affected);
        iListener.onFeatureMarkedOutdated(affected, cause);
    }
}
