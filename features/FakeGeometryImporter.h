#pragma once

#include "core/GeometryImport.h"

// Stand-in for a per-CAD importer (ADR-0001): builds a small fake BRep and
// the host-id correspondence a real importer would produce.
class FakeGeometryImporter : public IGeometryImporter
{
public:
    ImportOutcome import() override;
};
