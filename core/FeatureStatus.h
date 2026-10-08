#pragma once

// Lifecycle of a feature's result on a model. See SPEC.md section 4.
enum class FeatureStatus
{
    NeverRecognized, // no result
    Valid,           // has a result, trustworthy
    Outdated         // has a result, but an upstream changed since
};
