// The executable version of SPEC.md section 7: seven acceptance scenarios,
// plus registration validation (section 3.1) and failure semantics
// (section 3.7).
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "tests/doctest.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "app/InMemoryParameterStore.h"
#include "core/RecognitionEngine.h"
#include "features/AAGRecognizer.h"
#include "features/FakeGeometryImporter.h"
#include "features/FilletRecognizer.h"
#include "features/HoleRecognizer.h"
#include "features/RibRecognizer.h"

namespace
{

class RecordingListener : public IRecognitionListener
{
public:
    std::vector<std::string> started;
    std::vector<std::pair<std::string, std::string>> outdated; // affected, cause

    void onRecognitionStarted(const FeatureId &feature) override
    {
        started.push_back(feature.name());
    }
    void onRecognitionSucceeded(const FeatureId &, std::chrono::milliseconds) override {}
    void onRecognitionFailed(const FeatureId &, const std::string &) override {}
    void onFeatureMarkedOutdated(const FeatureId &affected, const FeatureId &cause) override
    {
        outdated.emplace_back(affected.name(), cause.name());
    }
};

FeatureId mustRegister(RecognitionEngine &engine, std::unique_ptr<IRecognizer> recognizer)
{
    auto result = engine.registerRecognizer(std::move(recognizer));
    REQUIRE_MESSAGE(result.id.has_value(), result.error);
    return *result.id;
}

// The four real recognizers, registered and finalized, geometry imported.
struct TestRig
{
    InMemoryParameterStore store;
    RecordingListener listener;
    RecognitionEngine engine;
    Model model;
    FeatureId aag;
    FeatureId fillet;
    FeatureId rib;
    FeatureId hole;

    explicit TestRig(std::unique_ptr<IRecognizer> extra = nullptr)
        : engine(store, listener), aag(mustRegister(engine, std::make_unique<AAGRecognizer>())),
          fillet(mustRegister(engine, std::make_unique<FilletRecognizer>())),
          rib(mustRegister(engine, std::make_unique<RibRecognizer>())),
          hole(mustRegister(engine, std::make_unique<HoleRecognizer>()))
    {
        if (extra)
            mustRegister(engine, std::move(extra));
        const auto finalized = engine.finalizeRegistration();
        REQUIRE_MESSAGE(finalized.ok, finalized.error);
        reimport();
    }

    void reimport()
    {
        auto outcome = FakeGeometryImporter().import();
        REQUIRE(outcome.succeeded);
        model.acceptImport(std::move(outcome.brep), std::move(outcome.ids));
    }

    RecognitionReport recognize(std::vector<FeatureId> targets)
    {
        RecognitionRequest request;
        request.targets = std::move(targets);
        return engine.recognize(model, request);
    }
};

} // namespace

TEST_CASE("Scenario 1: dependency graph is assembled from registration, not hardcoded")
{
    TestRig rig;
    CHECK(rig.engine.directUpstream(rig.aag).empty());
    CHECK(rig.engine.directUpstream(rig.fillet) == std::vector<FeatureId>{rig.aag});
    CHECK(rig.engine.directUpstream(rig.rib) == std::vector<FeatureId>{rig.fillet});
    CHECK(rig.engine.directUpstream(rig.hole) == std::vector<FeatureId>{rig.fillet});
}

TEST_CASE("Scenario 2: missing upstream is auto-recomputed, with live progress")
{
    TestRig rig;
    const auto report = rig.recognize({rig.fillet});

    REQUIRE(report.entries.size() == 2);
    CHECK(report.entries[0].feature == rig.aag);
    CHECK(report.entries[0].wasAutoAdded);
    CHECK(report.entries[1].feature == rig.fillet);
    CHECK_FALSE(report.entries[1].wasAutoAdded);
    CHECK(report.allSucceeded());

    // "recognizing aag" was reported live, before fillet started
    CHECK(rig.listener.started == std::vector<std::string>{"aag", "fillet"});
    CHECK(rig.model.status(rig.aag) == FeatureStatus::Valid);
    CHECK(rig.model.status(rig.fillet) == FeatureStatus::Valid);
}

TEST_CASE("Scenario 3: re-recognizing an upstream outdates every downstream")
{
    TestRig rig;
    rig.recognize({rig.aag, rig.fillet, rig.rib, rig.hole});
    REQUIRE(rig.model.status(rig.hole) == FeatureStatus::Valid);

    rig.recognize({rig.aag});

    CHECK(rig.model.status(rig.aag) == FeatureStatus::Valid);
    CHECK(rig.model.status(rig.fillet) == FeatureStatus::Outdated);
    CHECK(rig.model.status(rig.rib) == FeatureStatus::Outdated);
    CHECK(rig.model.status(rig.hole) == FeatureStatus::Outdated);
    // outdated results stay readable (SPEC section 3.5)
    CHECK(rig.model.result(rig.fillet) != nullptr);
    CHECK(rig.model.result(rig.rib) != nullptr);
}

TEST_CASE("Scenario 4: request order does not matter")
{
    TestRig rig;
    const auto report = rig.recognize({rig.fillet, rig.aag}); // deliberately reversed

    REQUIRE(report.entries.size() == 2);
    CHECK(report.entries[0].feature == rig.aag); // aag ran first anyway
    CHECK(report.entries[1].feature == rig.fillet);
    CHECK(report.allSucceeded());
}

namespace
{
// Scenario 5: this recognizer lives entirely outside core/ and features/.
// Adding a feature = one new file like this one + one registration line.
struct ChamferParameters : public IParameters
{
    double widthMm = 1.0;
};

class ChamferResult : public IFeatureResult
{
public:
    std::string describe() const override { return "Chamfer: fake"; }
};

class ChamferRecognizer : public IRecognizer
{
public:
    std::string name() const override { return "chamfer"; }
    std::vector<std::string> dependencies() const override { return {"fillet"}; }
    bool isUserVisible() const override { return true; }
    std::shared_ptr<const IParameters> defaultParameters() const override
    {
        return std::make_shared<ChamferParameters>();
    }
    RecognitionOutcome recognize(const RecognitionContext &context) override
    {
        if (!context.dependencyResult("fillet"))
            return RecognitionOutcome::failure("chamfer: fillet result unavailable");
        return RecognitionOutcome::success(std::make_unique<ChamferResult>());
    }
};
} // namespace

TEST_CASE("Scenario 5: a fifth feature plugs in without touching the core")
{
    TestRig rig(std::make_unique<ChamferRecognizer>());
    const auto chamfer = rig.engine.featureByName("chamfer");
    REQUIRE(chamfer.has_value());

    CHECK(rig.engine.directUpstream(*chamfer) == std::vector<FeatureId>{rig.fillet});

    const auto report = rig.recognize({*chamfer});
    REQUIRE(report.entries.size() == 3); // aag, fillet auto-recomputed
    CHECK(report.entries[0].feature == rig.aag);
    CHECK(report.entries[1].feature == rig.fillet);
    CHECK(report.entries[2].feature == *chamfer);
    CHECK(report.allSucceeded());
    CHECK(rig.model.status(*chamfer) == FeatureStatus::Valid);
}

TEST_CASE("Scenario 6: reimport clears every result but keeps parameters")
{
    TestRig rig;
    RecognitionRequest request;
    request.targets = {rig.fillet};
    auto parameters = std::make_shared<FilletParameters>();
    parameters->radiusMm = 3.0;
    request.parameters[rig.fillet] = parameters;
    rig.engine.recognize(rig.model, request);
    REQUIRE(rig.model.status(rig.fillet) == FeatureStatus::Valid);

    rig.reimport();

    for (const auto &feature : rig.engine.allFeatures())
    {
        CHECK(rig.model.status(feature) == FeatureStatus::NeverRecognized);
        CHECK(rig.model.result(feature) == nullptr);
    }
    // the parameter store survives reimport (SPEC section 3.6)
    CHECK(rig.store.lastUsedParameters(rig.fillet) != nullptr);
}

TEST_CASE("Scenario 7: auto-recompute reuses the last successfully used parameters")
{
    TestRig rig;

    // user ran fillet with radius 3 once
    RecognitionRequest request;
    request.targets = {rig.fillet};
    auto parameters = std::make_shared<FilletParameters>();
    parameters->radiusMm = 3.0;
    request.parameters[rig.fillet] = parameters;
    rig.engine.recognize(rig.model, request);

    // aag re-ran, fillet became outdated
    rig.recognize({rig.aag});
    REQUIRE(rig.model.status(rig.fillet) == FeatureStatus::Outdated);

    // user asks for rib only; fillet is auto-recomputed
    const auto report = rig.recognize({rig.rib});
    REQUIRE(report.entries.size() == 2);
    CHECK(report.entries[0].feature == rig.fillet);
    CHECK(report.entries[0].wasAutoAdded);
    CHECK(report.entries[1].feature == rig.rib);

    // with radius 3, not the default 1
    const auto *result = dynamic_cast<const FilletResult *>(rig.model.result(rig.fillet));
    REQUIRE(result != nullptr);
    CHECK(result->radiusUsedMm() == 3.0);
}

namespace
{
// Minimal recognizers for core-behavior tests, independent of features/.
class StubResult : public IFeatureResult
{
public:
    std::string describe() const override { return "stub"; }
};

class StubRecognizer : public IRecognizer
{
public:
    StubRecognizer(std::string name, std::vector<std::string> dependencies,
                   const bool *shouldFail = nullptr)
        : iName(std::move(name)), iDependencies(std::move(dependencies)), iShouldFail(shouldFail)
    {
    }

    std::string name() const override { return iName; }
    std::vector<std::string> dependencies() const override { return iDependencies; }
    bool isUserVisible() const override { return true; }
    std::shared_ptr<const IParameters> defaultParameters() const override
    {
        return std::make_shared<IParameters>();
    }
    RecognitionOutcome recognize(const RecognitionContext &) override
    {
        if (iShouldFail && *iShouldFail)
            return RecognitionOutcome::failure(iName + ": forced failure");
        return RecognitionOutcome::success(std::make_unique<StubResult>());
    }

private:
    std::string iName;
    std::vector<std::string> iDependencies;
    const bool *iShouldFail;
};

struct StubRig
{
    InMemoryParameterStore store;
    RecordingListener listener;
    RecognitionEngine engine{store, listener};
    Model model;

    void importGeometry()
    {
        auto outcome = FakeGeometryImporter().import();
        REQUIRE(outcome.succeeded);
        model.acceptImport(std::move(outcome.brep), std::move(outcome.ids));
    }
};

} // namespace

TEST_CASE("Registration: duplicate names fail fast")
{
    StubRig rig;
    mustRegister(rig.engine, std::make_unique<StubRecognizer>("a", std::vector<std::string>{}));
    const auto duplicate = rig.engine.registerRecognizer(
        std::make_unique<StubRecognizer>("a", std::vector<std::string>{}));
    CHECK_FALSE(duplicate.id.has_value());
    CHECK(duplicate.error.find("duplicate") != std::string::npos);
}

TEST_CASE("Registration: unknown dependency fails at finalize")
{
    StubRig rig;
    mustRegister(rig.engine,
                 std::make_unique<StubRecognizer>("a", std::vector<std::string>{"missing"}));
    const auto finalized = rig.engine.finalizeRegistration();
    CHECK_FALSE(finalized.ok);
    CHECK(finalized.error.find("missing") != std::string::npos);
}

TEST_CASE("Registration: dependency cycle fails at finalize")
{
    StubRig rig;
    mustRegister(rig.engine, std::make_unique<StubRecognizer>("a", std::vector<std::string>{"b"}));
    mustRegister(rig.engine, std::make_unique<StubRecognizer>("b", std::vector<std::string>{"a"}));
    const auto finalized = rig.engine.finalizeRegistration();
    CHECK_FALSE(finalized.ok);
    CHECK(finalized.error.find("cycle") != std::string::npos);
}

TEST_CASE("Failure semantics: failed upstream blocks downstream; state stays untouched")
{
    StubRig rig;
    bool midFails = true;
    const auto base = mustRegister(
        rig.engine, std::make_unique<StubRecognizer>("base", std::vector<std::string>{}));
    const auto mid = mustRegister(
        rig.engine,
        std::make_unique<StubRecognizer>("mid", std::vector<std::string>{"base"}, &midFails));
    const auto leaf = mustRegister(
        rig.engine, std::make_unique<StubRecognizer>("leaf", std::vector<std::string>{"mid"}));
    REQUIRE(rig.engine.finalizeRegistration().ok);
    rig.importGeometry();

    // fresh model: mid fails, leaf is skipped with the cause named
    RecognitionRequest request;
    request.targets = {leaf};
    auto report = rig.engine.recognize(rig.model, request);
    REQUIRE(report.entries.size() == 3);
    CHECK(report.entries[0].outcome == FeatureRunReport::Outcome::Recognized); // base
    CHECK(report.entries[1].outcome == FeatureRunReport::Outcome::Failed);     // mid
    CHECK(report.entries[2].outcome == FeatureRunReport::Outcome::Skipped);    // leaf
    CHECK(report.entries[2].detail.find("mid") != std::string::npos);
    CHECK(rig.model.status(mid) == FeatureStatus::NeverRecognized);
    CHECK(rig.model.status(leaf) == FeatureStatus::NeverRecognized);

    // let everything succeed once
    midFails = false;
    report = rig.engine.recognize(rig.model, request);
    CHECK(report.allSucceeded());
    REQUIRE(rig.model.status(leaf) == FeatureStatus::Valid);

    // a later failure leaves the old Valid result untouched and does not
    // outdate anything downstream (SPEC section 3.7)
    midFails = true;
    RecognitionRequest midOnly;
    midOnly.targets = {mid};
    report = rig.engine.recognize(rig.model, midOnly);
    REQUIRE(report.entries.size() == 1);
    CHECK(report.entries[0].outcome == FeatureRunReport::Outcome::Failed);
    CHECK(rig.model.status(mid) == FeatureStatus::Valid);
    CHECK(rig.model.result(mid) != nullptr);
    CHECK(rig.model.status(leaf) == FeatureStatus::Valid);
    CHECK(base == base); // silence unused warning politely
}
