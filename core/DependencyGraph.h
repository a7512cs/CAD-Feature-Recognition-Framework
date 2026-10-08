#pragma once

#include <unordered_map>
#include <vector>

#include "core/FeatureId.h"

// Directed dependency graph between features, assembled from what each
// recognizer declares at registration (ADR-0003). "Downstream depends on
// upstream" is the only relationship it stores.
class DependencyGraph
{
public:
    void addNode(const FeatureId &feature);
    void addEdge(const FeatureId &downstream, const FeatureId &upstream);

    const std::vector<FeatureId> &directUpstream(const FeatureId &feature) const;
    std::vector<FeatureId>
    upstreamClosure(const FeatureId &feature) const; // transitive, excludes self
    std::vector<FeatureId>
    downstreamClosure(const FeatureId &feature) const; // transitive, excludes self

    bool isAcyclic() const;

    // Orders the given features so every upstream comes before its
    // downstream. Ties resolve by registration order, so runs are
    // deterministic. Features whose dependencies lie outside the given set
    // are treated as ready.
    std::vector<FeatureId> topologicalOrder(std::vector<FeatureId> features) const;

private:
    std::vector<FeatureId>
    closure(const FeatureId &start,
            const std::unordered_map<FeatureId, std::vector<FeatureId>> &adjacency) const;

    std::vector<FeatureId> iNodes; // registration order
    std::unordered_map<FeatureId, std::vector<FeatureId>> iUpstream;
    std::unordered_map<FeatureId, std::vector<FeatureId>> iDownstream;
};
