#include "features/AAGRecognizer.h"

#include <sstream>

std::string AAGResult::describe() const
{
    std::ostringstream out;
    out << "AAG: " << iAdjacencies.size() << " adjacencies (angle=" << iAngleUsedDeg << " deg)";
    return out.str();
}

RecognitionOutcome AAGRecognizer::recognize(const RecognitionContext &context)
{
    const auto *parameters = dynamic_cast<const AAGParameters *>(&context.parameters());
    if (!parameters)
        return RecognitionOutcome::failure("aag: unexpected parameter type");

    const auto &faces = context.brep().faces;
    if (faces.empty())
        return RecognitionOutcome::failure("aag: model has no faces");

    // Fake: link consecutive faces, alternate convexity. Real algorithm TBD.
    std::vector<AAGAdjacency> adjacencies;
    for (std::size_t i = 0; i + 1 < faces.size(); ++i)
        adjacencies.push_back({faces[i].id, faces[i + 1].id, i % 2 == 0});

    return RecognitionOutcome::success(
        std::make_unique<AAGResult>(parameters->angleThresholdDeg, std::move(adjacencies)));
}
