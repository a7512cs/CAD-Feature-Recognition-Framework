#include "core/DependencyGraph.h"

#include <algorithm>
#include <unordered_set>

void DependencyGraph::addNode(const FeatureId &feature)
{
    if (iUpstream.count(feature))
        return;
    iNodes.push_back(feature);
    iUpstream[feature];
    iDownstream[feature];
}

void DependencyGraph::addEdge(const FeatureId &downstream, const FeatureId &upstream)
{
    addNode(downstream);
    addNode(upstream);
    iUpstream[downstream].push_back(upstream);
    iDownstream[upstream].push_back(downstream);
}

const std::vector<FeatureId> &DependencyGraph::directUpstream(const FeatureId &feature) const
{
    static const std::vector<FeatureId> empty;
    auto it = iUpstream.find(feature);
    return it == iUpstream.end() ? empty : it->second;
}

std::vector<FeatureId> DependencyGraph::upstreamClosure(const FeatureId &feature) const
{
    return closure(feature, iUpstream);
}

std::vector<FeatureId> DependencyGraph::downstreamClosure(const FeatureId &feature) const
{
    return closure(feature, iDownstream);
}

std::vector<FeatureId> DependencyGraph::closure(
    const FeatureId &start,
    const std::unordered_map<FeatureId, std::vector<FeatureId>> &adjacency) const
{
    std::vector<FeatureId> result;
    std::unordered_set<FeatureId> visited{start};
    std::vector<FeatureId> pending{start};
    while (!pending.empty())
    {
        const FeatureId current = pending.back();
        pending.pop_back();
        auto it = adjacency.find(current);
        if (it == adjacency.end())
            continue;
        for (const auto &next : it->second)
        {
            if (visited.insert(next).second)
            {
                result.push_back(next);
                pending.push_back(next);
            }
        }
    }
    return result;
}

bool DependencyGraph::isAcyclic() const
{
    return topologicalOrder(iNodes).size() == iNodes.size();
}

std::vector<FeatureId> DependencyGraph::topologicalOrder(std::vector<FeatureId> features) const
{
    // Kahn's algorithm over the induced subgraph, smallest registration
    // index first for a stable order. Sizes are tiny; O(n^2) is fine.
    std::sort(features.begin(), features.end(),
              [](const FeatureId &a, const FeatureId &b) { return a.index() < b.index(); });

    std::unordered_set<FeatureId> inSet(features.begin(), features.end());
    std::unordered_map<FeatureId, std::size_t> remaining; // unresolved in-set upstream edges
    for (const auto &feature : features)
    {
        std::size_t count = 0;
        for (const auto &up : directUpstream(feature))
            if (inSet.count(up))
                ++count;
        remaining[feature] = count;
    }

    std::vector<FeatureId> order;
    std::unordered_set<FeatureId> done;
    while (order.size() < features.size())
    {
        bool progressed = false;
        for (const auto &feature : features)
        {
            if (done.count(feature) || remaining[feature] != 0)
                continue;
            order.push_back(feature);
            done.insert(feature);
            progressed = true;
            auto it = iDownstream.find(feature);
            if (it == iDownstream.end())
                continue;
            for (const auto &down : it->second)
                if (inSet.count(down) && remaining[down] > 0)
                    --remaining[down];
        }
        if (!progressed)
            break; // the rest form a cycle
    }
    return order;
}
