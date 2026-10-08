#pragma once

#include <iostream>

#include "core/IRecognitionListener.h"

// Prints live progress to the console. A real host routes these to its own
// message window instead.
class ConsoleListener : public IRecognitionListener
{
public:
    void onRecognitionStarted(const FeatureId &feature) override
    {
        std::cout << "▶ 正在辨識 " << feature.name() << "...\n";
    }

    void onRecognitionSucceeded(const FeatureId &feature,
                                std::chrono::milliseconds elapsed) override
    {
        std::cout << "  ✓ " << feature.name() << " 完成（" << elapsed.count() << " ms）\n";
    }

    void onRecognitionFailed(const FeatureId &feature, const std::string &error) override
    {
        std::cout << "  ✗ " << feature.name() << " 失敗：" << error << "\n";
    }

    void onFeatureMarkedOutdated(const FeatureId &affected, const FeatureId &cause) override
    {
        std::cout << "  ⚠ " << affected.name() << " 已標記為過時（因 " << cause.name()
                  << " 重新辨識）\n";
    }
};
