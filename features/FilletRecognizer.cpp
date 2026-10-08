#include "features/FilletRecognizer.h"

#include <sstream>

#include "features/AAGRecognizer.h"

std::string FilletResult::describe() const
{
    std::ostringstream out;
    out << "Fillet: " << iInstances.size() << " instances (radius=" << iRadiusUsedMm << " mm)";
    return out.str();
}

RecognitionOutcome FilletRecognizer::recognize(const RecognitionContext &context)
{
    const auto *parameters = dynamic_cast<const FilletParameters *>(&context.parameters());
    if (!parameters)
        return RecognitionOutcome::failure("fillet: unexpected parameter type");

    const auto *aag = dynamic_cast<const AAGResult *>(context.dependencyResult("aag"));
    if (!aag)
        return RecognitionOutcome::failure("fillet: aag result unavailable");

    // Fake: every concave adjacency is a fillet candidate. Real algorithm TBD.
    std::vector<FilletInstance> instances;
    int index = 0;
    for (const auto &adjacency : aag->adjacencies())
    {
        if (adjacency.isConvex)
            continue;
        instances.push_back({index++, parameters->radiusMm, {adjacency.faceA, adjacency.faceB}});
    }

    return RecognitionOutcome::success(
        std::make_unique<FilletResult>(parameters->radiusMm, std::move(instances)));
}
