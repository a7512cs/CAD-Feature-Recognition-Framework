#pragma once

#include <string>
#include <unordered_map>

#include "core/BRep.h"

// Our face id <-> host CAD persistent id, built once at import
// (SPEC section 3.2). Faces only for the POC; extend per topology kind
// when a real importer lands.
struct IdCorrespondence
{
    std::unordered_map<int, std::string> faceToHost;
    std::unordered_map<std::string, int> hostToFace;
};

struct ImportOutcome
{
    bool succeeded = false;
    std::string error;
    BRep brep;
    IdCorrespondence ids;
};

// One implementation per host CAD. Converts the host model into our BRep
// in one pass; recognition never queries the host afterwards (ADR-0001).
class IGeometryImporter
{
public:
    virtual ~IGeometryImporter() = default;
    virtual ImportOutcome import() = 0;
};
