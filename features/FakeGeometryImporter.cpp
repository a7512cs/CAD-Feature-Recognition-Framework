#include "features/FakeGeometryImporter.h"

#include <string>

ImportOutcome FakeGeometryImporter::import()
{
    ImportOutcome outcome;

    BRep brep;
    for (int i = 0; i < 6; ++i)
    {
        Face face;
        face.id = i;
        face.surface = (i % 2 == 0) ? SurfaceType::Plane : SurfaceType::Cylinder;
        face.area = 10.0 * (i + 1);
        brep.faces.push_back(face);
    }
    for (int i = 0; i < 8; ++i)
        brep.edges.push_back({i});
    for (int i = 0; i < 2; ++i)
        brep.trims.push_back({i});
    for (int i = 0; i < 4; ++i)
        brep.vertices.push_back({i});

    IdCorrespondence ids;
    for (const auto &face : brep.faces)
    {
        const std::string hostId = "host-face-" + std::to_string(face.id);
        ids.faceToHost[face.id] = hostId;
        ids.hostToFace[hostId] = face.id;
    }

    outcome.brep = std::move(brep);
    outcome.ids = std::move(ids);
    outcome.succeeded = true;
    return outcome;
}
