#ifndef UPDATE_STRATEGY_H
#define UPDATE_STRATEGY_H

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QTextStream>
#include <optional>

#include "logger/logger.h"
#include "utils.hpp"

namespace otalib {

constexpr auto kStrategyFile = "./strategy.json";
constexpr bool kKeepFileOpen = false;

enum class StrategyAction { update, rollback };
enum class StrategyType { optional, compulsory, error };

using CheckInfo = std::tuple<StrategyAction, StrategyType, QString>;
// 1: Get client infos in jdoc_client.
// 2: Check the strategy by using infos given by the client.
// 3: Return the target pack directory according to the strategy.
::std::optional<CheckInfo> updateStrategyCheck(const QByteArray& raw);

}  // namespace otalib

#endif  // UPDATE_STRATEGY_H
