#pragma once

#include <memory>
#include <string>
#include <vector>

#include "core/GeometryImport.h"
#include "core/Model.h"
#include "core/RecognitionEngine.h"

// Console front end. This is a "typed end": it knows the concrete parameter
// types, so it can turn "fillet=3" into FilletParameters.
class ConsoleUi
{
public:
    ConsoleUi(RecognitionEngine &engine, Model &model, IGeometryImporter &importer)
        : iEngine(engine), iModel(model), iImporter(importer)
    {
    }

    int run();

private:
    void printHelp() const;
    void printStatus() const;
    void handleRecognize(const std::vector<std::string> &args);
    void handleReimport();
    std::shared_ptr<const IParameters> makeParameters(const std::string &featureName,
                                                      double value) const;
    void printReport(const RecognitionReport &report) const;

    RecognitionEngine &iEngine;
    Model &iModel;
    IGeometryImporter &iImporter;
};
