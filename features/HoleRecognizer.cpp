#include "features/HoleRecognizer.h"

#include <sstream>
#include <unordered_set>

#include "features/FilletRecognizer.h"

std::string HoleResult::describe() const
{
    std::ostringstream out;
    out << "Hole: " << iInstances.size() << " instances (radius=" << iRadiusUsedMm << " mm)";
    return out.str();
}

RecognitionOutcome HoleRecognizer::recognize(const RecognitionContext &context)
{
    const auto *parameters = dynamic_cast<const HoleParameters *>(&context.parameters());
    if (!parameters)
        return RecognitionOutcome::failure("hole: unexpected parameter type");

    const auto *fillet = dynamic_cast<const FilletResult *>(context.dependencyResult("fillet"));
    if (!fillet)
        return RecognitionOutcome::failure("hole: fillet result unavailable");

    // Fake: every cylindrical face not already claimed by a fillet is a
    // hole. Real algorithm TBD.
    std::unordered_set<int> claimedFaceIds;
    for (const auto &filletInstance : fillet->instances())
        claimedFaceIds.insert(filletInstance.faceIds.begin(), filletInstance.faceIds.end());

    std::vector<HoleInstance> instances;
    int index = 0;
    for (const auto &face : context.brep().faces)
    {
        if (face.surface != SurfaceType::Cylinder || claimedFaceIds.count(face.id))
            continue;
        instances.push_back({index++, parameters->radiusMm, {face.id}});
    }

    return RecognitionOutcome::success(
        std::make_unique<HoleResult>(parameters->radiusMm, std::move(instances)));
}
