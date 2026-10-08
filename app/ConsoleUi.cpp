#include "app/ConsoleUi.h"

#include <iostream>
#include <sstream>

#include "features/AAGRecognizer.h"
#include "features/FilletRecognizer.h"
#include "features/HoleRecognizer.h"
#include "features/RibRecognizer.h"

namespace
{
const char *statusLabel(FeatureStatus status)
{
    switch (status)
    {
    case FeatureStatus::NeverRecognized:
        return "尚未辨識";
    case FeatureStatus::Valid:
        return "有效";
    case FeatureStatus::Outdated:
        return "已過時";
    }
    return "?";
}
} // namespace

int ConsoleUi::run()
{
    std::cout << "=== DemoFR — Feature Recognition POC ===\n";
    handleReimport();
    printHelp();

    std::string line;
    while (true)
    {
        std::cout << "\n> " << std::flush;
        if (!std::getline(std::cin, line))
            break;

        std::istringstream stream(line);
        std::string command;
        stream >> command;
        std::vector<std::string> args;
        std::string arg;
        while (stream >> arg)
            args.push_back(arg);

        if (command.empty())
            continue;
        if (command == "quit" || command == "exit")
            break;
        if (command == "help")
            printHelp();
        else if (command == "status")
            printStatus();
        else if (command == "reimport")
            handleReimport();
        else if (command == "recognize")
            handleRecognize(args);
        else
            std::cout << "未知指令：" << command << "（輸入 help 查看用法）\n";
    }
    return 0;
}

void ConsoleUi::printHelp() const
{
    std::cout << "指令：\n"
              << "  recognize <feature>[=<value>] ...   辨識特徵，可帶參數（例：recognize fillet=3 "
                 "rib）\n"
              << "  status                              顯示所有特徵的狀態與結果\n"
              << "  reimport                            重新匯入模型（清空所有辨識結果）\n"
              << "  quit                                離開\n"
              << "特徵：";
    for (const auto &feature : iEngine.allFeatures())
    {
        std::cout << " " << feature.name();
        if (!iEngine.isUserVisible(feature))
            std::cout << "(internal)";
    }
    std::cout << "\n";
}

void ConsoleUi::printStatus() const
{
    if (iModel.hasGeometry())
        std::cout << "模型：" << iModel.brep().faces.size() << " faces\n";
    else
        std::cout << "模型：尚未匯入\n";

    for (const auto &feature : iEngine.allFeatures())
    {
        std::cout << "  " << feature.name();
        if (!iEngine.isUserVisible(feature))
            std::cout << "（internal）";
        std::cout << " — " << statusLabel(iModel.status(feature));
        if (const auto *result = iModel.result(feature))
            std::cout << " — " << result->describe();
        std::cout << "\n";
    }
}

void ConsoleUi::handleRecognize(const std::vector<std::string> &args)
{
    if (args.empty())
    {
        std::cout << "用法：recognize <feature>[=<value>] ...\n";
        return;
    }

    RecognitionRequest request;
    for (const auto &arg : args)
    {
        std::string name = arg;
        std::string value;
        if (const auto pos = arg.find('='); pos != std::string::npos)
        {
            name = arg.substr(0, pos);
            value = arg.substr(pos + 1);
        }

        const auto id = iEngine.featureByName(name);
        if (!id)
        {
            std::cout << "[警告] 未知的 feature：" << name << "，已略過\n";
            continue;
        }
        request.targets.push_back(*id);

        if (value.empty())
            continue;
        try
        {
            const double parsed = std::stod(value);
            if (auto parameters = makeParameters(name, parsed))
                request.parameters[*id] = std::move(parameters);
            else
                std::cout << "[警告] " << name << " 沒有可用的參數輸入，忽略 =" << value << "\n";
        }
        catch (...)
        {
            std::cout << "[警告] 無法解析數值：" << value << "\n";
        }
    }

    if (request.targets.empty())
        return;
    printReport(iEngine.recognize(iModel, request));
}

void ConsoleUi::handleReimport()
{
    auto outcome = iImporter.import();
    if (!outcome.succeeded)
    {
        std::cout << "[錯誤] 匯入失敗：" << outcome.error << "\n";
        return;
    }
    const auto faceCount = outcome.brep.faces.size();
    iModel.acceptImport(std::move(outcome.brep), std::move(outcome.ids));
    std::cout << "已匯入模型（" << faceCount << " faces）。所有辨識結果已清空。\n";
}

std::shared_ptr<const IParameters> ConsoleUi::makeParameters(const std::string &featureName,
                                                             double value) const
{
    if (featureName == "aag")
    {
        auto parameters = std::make_shared<AAGParameters>();
        parameters->angleThresholdDeg = value;
        return parameters;
    }
    if (featureName == "fillet")
    {
        auto parameters = std::make_shared<FilletParameters>();
        parameters->radiusMm = value;
        return parameters;
    }
    if (featureName == "rib")
    {
        auto parameters = std::make_shared<RibParameters>();
        parameters->lengthMm = value;
        return parameters;
    }
    if (featureName == "hole")
    {
        auto parameters = std::make_shared<HoleParameters>();
        parameters->radiusMm = value;
        return parameters;
    }
    return nullptr;
}

void ConsoleUi::printReport(const RecognitionReport &report) const
{
    if (!report.globalError.empty())
    {
        std::cout << "[錯誤] " << report.globalError << "\n";
        return;
    }
    std::cout << "\n--- 辨識報告 ---\n";
    for (const auto &entry : report.entries)
    {
        const char *label = entry.outcome == FeatureRunReport::Outcome::Recognized ? "完成"
                            : entry.outcome == FeatureRunReport::Outcome::Skipped  ? "跳過"
                                                                                   : "失敗";
        std::cout << "  " << entry.feature.name() << "：" << label;
        if (entry.wasAutoAdded)
            std::cout << "（自動補算上游）";
        if (entry.outcome == FeatureRunReport::Outcome::Recognized)
            std::cout << "（" << entry.elapsed.count() << " ms）";
        if (!entry.detail.empty())
            std::cout << " — " << entry.detail;
        std::cout << "\n";
    }
}
