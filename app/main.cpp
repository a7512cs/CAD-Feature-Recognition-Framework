#include <iostream>
#include <memory>

#include "app/ConsoleListener.h"
#include "app/ConsoleUi.h"
#include "app/InMemoryParameterStore.h"
#include "core/RecognitionEngine.h"
#include "features/AAGRecognizer.h"
#include "features/FakeGeometryImporter.h"
#include "features/FilletRecognizer.h"
#include "features/HoleRecognizer.h"
#include "features/RibRecognizer.h"

int main()
{
    InMemoryParameterStore parameterStore;
    ConsoleListener listener;
    RecognitionEngine engine(parameterStore, listener);

    // Composition root. Adding a feature = one new file in features/ plus
    // one line here (ADR-0003).
    std::unique_ptr<IRecognizer> recognizers[] = {
        std::make_unique<AAGRecognizer>(),
        std::make_unique<FilletRecognizer>(),
        std::make_unique<RibRecognizer>(),
        std::make_unique<HoleRecognizer>(),
    };
    for (auto &recognizer : recognizers)
    {
        auto registered = engine.registerRecognizer(std::move(recognizer));
        if (!registered.id)
        {
            std::cerr << "註冊失敗：" << registered.error << "\n";
            return 1;
        }
    }

    const auto finalized = engine.finalizeRegistration();
    if (!finalized.ok)
    {
        std::cerr << "註冊失敗：" << finalized.error << "\n";
        return 1;
    }

    FakeGeometryImporter importer;
    Model model;
    ConsoleUi ui(engine, model, importer);
    return ui.run();
}
