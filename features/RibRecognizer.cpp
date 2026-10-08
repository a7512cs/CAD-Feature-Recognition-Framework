#include "features/RibRecognizer.h"

#include <sstream>

#include "features/FilletRecognizer.h"

std::string RibResult::describe() const
{
    std::ostringstream out;
    out << "Rib: " << iInstances.size() << " instances (length=" << iLengthUsedMm << " mm)";
    return out.str();
}

RecognitionOutcome RibRecognizer::recognize(const RecognitionContext &context)
{
    const auto *parameters = dynamic_cast<const RibParameters *>(&context.parameters());
    if (!parameters)
        return RecognitionOutcome::failure("rib: unexpected parameter type");

    const auto *fillet = dynamic_cast<const FilletResult *>(context.dependencyResult("fillet"));
    if (!fillet)
        return RecognitionOutcome::failure("rib: fillet result unavailable");

    // Fake: one rib alongside every fillet. Real algorithm TBD.
    std::vector<RibInstance> instances;
    int index = 0;
    for (const auto &filletInstance : fillet->instances())
        instances.push_back({index++, parameters->lengthMm, filletInstance.faceIds});

    return RecognitionOutcome::success(
        std::make_unique<RibResult>(parameters->lengthMm, std::move(instances)));
}
