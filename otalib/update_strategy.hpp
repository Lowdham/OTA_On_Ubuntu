#ifndef UPDATE_STRATEGY_HPP
#define UPDATE_STRATEGY_HPP

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
#include "vcm.hpp"

namespace otalib {

constexpr auto kStrategyFile = "./strategy.json";
constexpr bool kKeepFileOpen = false;

enum class StrategyAction { update, rollback };
enum class StrategyType { optional, compulsory, error };
enum class sAction { Update, Rollback, None };
enum class JsonType { Request, Response, Confirm, Error };
static const QString kRequestField = QStringLiteral("Request");
static const QString kResponseField = QStringLiteral("Response");
static const QString kConfirmField = QStringLiteral("Confirm");
/*
Request Json Pattern
{
   "Request" = "Yes",
   "name" = "...",
   "version" = "...",
   "type" = "..."
}
Response Json Pattern
{
   "Response" = "Yes",
   "Action" = "...",
   "Strategy" = "...",
   "Destination" = "..."
}
*/
template <typename VersionType>
using CheckInfo = std::tuple<StrategyAction, StrategyType, VersionType>;
template <typename VersionType>
using RequestResponse =
    ::std::tuple<sAction, StrategyType, VersionType, VersionType>;
namespace {
// Strategy Pattern
/*
[
  {
    "name" : "XXX"
    "rollback" : [...],
    "update" : [...]
  },
  ...
]
// Block Pattern
"version" : {
  ## 1
  "compare" = "</<=/==/!=/>=/>",

## 1
    "value" = "XXX"
}

"type" : {
    ## 1
        "assign" : "=/!=",

                   ## 1
                   "name" : "XXX"
}

"condition" : {
    ## 0..1
    "version" : {...},

    ## 0..1
    "type" : {...}
}
              */
/*
"rollback" : [
{
## 1
"strategy" = "optional/compulsory",
## 0..1
"condition" = {...},
## 1
"dest" = "XXX"
},
...
]

"update" : [
    {
        ## 1
        "strategy" = "optional/compulsory",
        ## 0..1
        "condition" = {...},
        ## 1
        "dest" = "XXX"
    },
    ...
]
           NOTE: "optional" means the client can decide whether update/rollback
or not. "compulsory" means the client must update/rollback.
*/
enum class Compare { lt, lte, gt, gte, eq, neq, error };

template <typename VersionType>
struct ClientInfo {
  QString name;
  VersionType version;
  QString type;
};

template <typename VersionType>
struct VersionConditionBlock {
  Compare cmp;
  VersionType value;
};

struct TypeConditionBlock {
  Compare assign;  // only allow "eq" and "neq".
  QString name;
};

template <typename VersionType>
struct ConditionBlock {
  std::optional<VersionConditionBlock<VersionType>> vcond;
  std::optional<TypeConditionBlock> tcond;
};

template <typename VersionType>
struct UpdateBlock {
  StrategyType stg;
  std::optional<ConditionBlock<VersionType>> cond;
  VersionType dest;
};

template <typename VersionType>
struct RollbackBlock {
  StrategyType stg;
  std::optional<ConditionBlock<VersionType>> cond;
  VersionType dest;
};

template <typename VersionType>
VersionConditionBlock<VersionType> receiveVersionCond(
    const QJsonObject& object) {
  VersionConditionBlock<VersionType> vcb;
  if (object.contains("compare") && object.value("compare").isString()) {
    if (object.value("compare").toString() == "==")
      vcb.cmp = Compare::eq;
    else if (object.value("compare").toString() == "!=")
      vcb.cmp = Compare::neq;
    else if (object.value("compare").toString() == "<")
      vcb.cmp = Compare::lt;
    else if (object.value("compare").toString() == "<=")
      vcb.cmp = Compare::lte;
    else if (object.value("compare").toString() == ">")
      vcb.cmp = Compare::gt;
    else if (object.value("compare").toString() == ">=")
      vcb.cmp = Compare::gte;
    else
      vcb.cmp = Compare::error;
  } else
    vcb.cmp = Compare::error;

  if constexpr (::std::is_constructible_v<VersionType, QString>) {
    if (object.contains("value") && object.value("value").isString())
      vcb.value = VersionType(object.value("value").toString());
    else
      vcb.value = VersionType();
  }
  if constexpr (::std::is_constructible_v<VersionType, double>) {
    if (object.contains("value") && object.value("value").isDouble())
      vcb.value = VersionType(object.value("value").toDouble());
    else
      vcb.value = VersionType();
  }

  return vcb;
}

TypeConditionBlock receiveTypeCond(const QJsonObject& object) {
  TypeConditionBlock tcb;
  if (object.contains("assign") && object.value("assign").isString()) {
    if (object.value("assign").toString() == "==")
      tcb.assign = Compare::eq;
    else if (object.value("assign").toString() == "!=")
      tcb.assign = Compare::neq;
    else
      tcb.assign = Compare::error;
  } else
    tcb.assign = Compare::error;

  if (object.contains("name") && object.value("name").isString())
    tcb.name = object.value("name").toString();
  else
    tcb.name = QString();

  return tcb;
}

template <typename VersionType>
ConditionBlock<VersionType> receiveConditionBlock(const QJsonObject& object) {
  ConditionBlock<VersionType> cond;

  if (object.contains("version") && object.value("version").isObject()) {
    cond.vcond =
        receiveVersionCond<VersionType>(object.value("version").toObject());
  } else
    cond.vcond = ::std::nullopt;

  if (object.contains("type") && object.value("type").isObject()) {
    cond.tcond = receiveTypeCond(object.value("type").toObject());
  } else
    cond.tcond = ::std::nullopt;

  return cond;
}

template <typename ActionBlock, typename VersionType>
ActionBlock receiveActionBlock(const QJsonObject& object) {
  ActionBlock ab;

  if (object.contains("strategy") && object.value("strategy").isString()) {
    if (object.value("strategy").toString() == "optional")
      ab.stg = StrategyType::optional;
    else if (object.value("strategy").toString() == "compulsory")
      ab.stg = StrategyType::compulsory;
    else
      ab.stg = StrategyType::error;
  } else
    ab.stg = StrategyType::error;

  if (object.contains("condition") && object.value("condition").isObject())
    ab.cond = receiveConditionBlock<VersionType>(
        object.value("condition").toObject());
  else
    ab.cond = ::std::nullopt;

  if constexpr (::std::is_constructible_v<VersionType, QString>) {
    if (object.contains("dest") && object.value("dest").isString())
      ab.dest = VersionType(object.value("dest").toString());
    else
      ab.dest = VersionType();
  }
  if constexpr (::std::is_constructible_v<VersionType, double>) {
    if (object.contains("dest") && object.value("dest").isDouble())
      ab.dest = VersionType(object.value("dest").toDouble());
    else
      ab.dest = VersionType();
  }

  return ab;
}

QJsonArray& getServerStrategy() {
  static QByteArray prev_hash_val;
  static QJsonArray prev_strategy;
  auto clearStrategy = [&] {
    while (!prev_strategy.isEmpty()) prev_strategy.removeFirst();
  };

  //
  QCryptographicHash hash(QCryptographicHash::Md5);
  QFile sfile(kStrategyFile);
  if (sfile.open(QFile::ReadOnly)) {
    QByteArray buffer_json = sfile.readAll();
    sfile.close();
    hash.addData(buffer_json);
    QByteArray hash_result = hash.result();
    if (hash_result == prev_hash_val) {
      // The file doesn't change.
      return prev_strategy;
    } else {
      // File has changed.
      QJsonParseError err;
      QJsonDocument doc = QJsonDocument::fromJson(buffer_json, &err);
      if (err.error != QJsonParseError::NoError || !doc.isArray()) {
        // Error occurs. Return a empty QJsonArray.
        print<GeneralFerrorCtrl>(std::cerr,
                                 "Connot read the update strategy file.");
        clearStrategy();
        return prev_strategy;
      }
      prev_strategy = doc.array();
      prev_hash_val = ::std::move(hash_result);
      return prev_strategy;
    }
  } else {
    // Cannot open the file.
    // Default solution is to return the prev_strategy.
    return prev_strategy;
  }
}

template <typename VersionType>
ClientInfo<VersionType> getClientInfo(const QJsonObject& cobj) {
  //
  ClientInfo<VersionType> ret;
  if (cobj.contains("name") && cobj.value("name").isString())
    ret.name = cobj.value("name").toString();
  else
    ret.name = QString();

  if constexpr (::std::is_constructible_v<VersionType, QString>) {
    if (cobj.contains("version") && cobj.value("version").isString())
      ret.version = VersionType(cobj.value("version").toString());
    else
      ret.version = VersionType();
  }

  if constexpr (::std::is_constructible_v<VersionType, double>) {
    if (cobj.contains("version") && cobj.value("version").isDouble())
      ret.version = VersionType(cobj.value("version").toDouble());
    else
      ret.version = VersionType();
  }

  if (cobj.contains("type") && cobj.value("type").isString())
    ret.type = cobj.value("type").toString();
  else
    ret.type = QString();

  return ret;
}

template <typename VersionType>
bool checkCondition(const ConditionBlock<VersionType>& cond,
                    const ClientInfo<VersionType>& cinfo) {
  if (cond.tcond.has_value()) {
    // Do type check.
    auto tcond = cond.tcond.value();
    if (tcond.assign == Compare::eq && tcond.name != cinfo.type)
      return false;
    else if (tcond.assign == Compare::neq && tcond.name == cinfo.type)
      return false;
    else {
      // TODO error.
      return true;
    }
  }

  if (cond.vcond.has_value()) {
    // Do version check.
    auto vcond = cond.vcond.value();
    if (vcond.cmp == Compare::eq)
      return cinfo.version == vcond.value;
    else if (vcond.cmp == Compare::neq)
      return cinfo.version != vcond.value;
    else if (vcond.cmp == Compare::lt)
      return cinfo.version < vcond.value;
    else if (vcond.cmp == Compare::lte)
      return cinfo.version <= vcond.value;
    else if (vcond.cmp == Compare::gt)
      return cinfo.version > vcond.value;
    else if (vcond.cmp == Compare::gte)
      return cinfo.version >= vcond.value;
  }
  return true;
}

template <typename VersionType>
::std::optional<CheckInfo<VersionType>> applyStrategy(
    const QJsonObject& strategy, const ClientInfo<VersionType>& cinfo,
    const VersionMap<VersionType>& vcm) {
  auto checkVaildity = [](const auto& block) {
    if (block.stg == StrategyType::error)
      return false;
    else
      return true;
  };

  // Check whether the responce is always the newest version.
  if (strategy.contains("newest")) {
    // Always return the "newest".
    return CheckInfo<VersionType>{StrategyAction::update,
                                  StrategyType::optional, vcm.newest()};
  }

  // Check update strategy.
  if (strategy.contains("update") && strategy.value("update").isArray()) {
    auto update_list = strategy.value("update").toArray();
    for (auto iter : update_list) {
      UpdateBlock<VersionType> ud =
          receiveActionBlock<UpdateBlock<VersionType>, VersionType>(
              iter.toObject());
      if (!ud.cond.has_value() ||
          (ud.cond.has_value() &&
           checkCondition<VersionType>(ud.cond.value(), cinfo))) {
        if (checkVaildity(ud))
          return CheckInfo<VersionType>{StrategyAction::update, ud.stg,
                                        ud.dest};
        else
          return ::std::nullopt;
      }
    }
  }

  if (strategy.contains("rollback") && strategy.value("rollback").isArray()) {
    auto rollback_list = strategy.value("rollback").toArray();
    for (auto iter : rollback_list) {
      RollbackBlock<VersionType> rb =
          receiveActionBlock<RollbackBlock<VersionType>, VersionType>(
              iter.toObject());
      if (!rb.cond.has_value() ||
          (rb.cond.has_value() &&
           checkCondition<VersionType>(rb.cond.value(), cinfo))) {
        if (checkVaildity(rb))
          return CheckInfo<VersionType>{StrategyAction::rollback, rb.stg,
                                        rb.dest};
        else
          return ::std::nullopt;
      }
    }
  }

  // No matched action.
  return ::std::nullopt;
}

template <typename VersionType>
::std::optional<CheckInfo<VersionType>> matchStrategy(
    const ClientInfo<VersionType>& cinfo, const VersionMap<VersionType>& vcm) {
  //
  const QJsonArray& strategy = getServerStrategy();
  for (auto iter : strategy) {
    if (iter.isObject()) {
      QJsonObject current = iter.toObject();
      if (current.contains("name") && current.value("name").isString() &&
          current.value("name").toString() == cinfo.name) {
        return applyStrategy<VersionType>(current, cinfo,
                                          vcm);  // Match succeed.
      } else
        continue;  // Match failed.
    } else {
      // error, just ignore.
      continue;
    }
  }
  return ::std::nullopt;
}

}  // namespace
// 1: Get client infos in jdoc_client.
// 2: Check the strategy by using infos given by the client.
// 3: Return the target pack directory according to the strategy.
template <typename VersionType>
::std::optional<CheckInfo<VersionType>> updateStrategyCheck(
    const QByteArray& raw, const VersionMap<VersionType>& vcm) {
  //
  QJsonParseError err;
  QJsonDocument doc = QJsonDocument::fromJson(raw, &err);
  if (err.error == QJsonParseError::NoError && doc.isObject())
    return ::std::nullopt;
  return matchStrategy<VersionType>(getClientInfo<VersionType>(doc.object()),
                                    vcm);
}

static QJsonDocument MakeRequest(const QString& appname, const QString& appver,
                                 const QString& apptype) {
  QJsonObject jobj;
  jobj["name"] = appname;
  jobj["version"] = appver;
  jobj["type"] = apptype;
  jobj[kRequestField] = QStringLiteral("Yes");

  QJsonDocument jdoc;
  jdoc.setObject(jobj);
  return jdoc;
}

namespace {
//
template <typename VersionType>
::std::optional<RequestResponse<VersionType>> DoParseResponse(
    const QJsonDocument& jdoc) {
  //
  QJsonObject jobj = jdoc.object();
  // accept "Action" field.
  if (!jobj.contains("Action")) return ::std::nullopt;
  QString action_field = jobj.find("Action").value().toString();
  if (action_field == "None")
    return RequestResponse<VersionType>{sAction::None, StrategyType::error,
                                        VersionType(), VersionType()};

  sAction act = sAction::None;
  if (action_field == "Update")
    act = sAction::Update;
  else if (action_field == "Rollback")
    act = sAction::Rollback;
  else
    return ::std::nullopt;

  // accept "Strategy" field.
  if (!jobj.contains("Strategy")) return ::std::nullopt;
  QString stg_field = jobj.find("Strategy").value().toString();
  StrategyType stg = StrategyType::error;
  if (stg_field == "Optional")
    stg = StrategyType::optional;
  else if (stg_field == "Compulsory")
    stg = StrategyType::compulsory;
  else
    return ::std::nullopt;

  // accept "From" field.
  if (!jobj.contains("From")) return ::std::nullopt;
  VersionType from{jobj.find("From").value().toString()};
  // accept "Destination" field.
  if (!jobj.contains("Destination")) return ::std::nullopt;
  VersionType dest{jobj.find("Destination").value().toString()};
  return RequestResponse<VersionType>{act, stg, from, dest};
}

}  // namespace

template <typename VersionType>
::std::optional<RequestResponse<VersionType>> ParseResponse(
    const QByteArray& raw) {
  //
  QJsonParseError err;
  QJsonDocument jdoc = QJsonDocument::fromJson(raw, &err);
  if (err.error != QJsonParseError::NoError || !jdoc.isObject())
    return ::std::nullopt;
  return DoParseResponse<VersionType>(jdoc);
}

template <typename VersionType>
QJsonDocument MakeResponse(sAction act, StrategyType stg, VersionType& from,
                           VersionType* dest = nullptr) {
  //
  QJsonDocument jdoc;
  QJsonObject jobj;
  switch (act) {
    case sAction::None:
      jobj["Action"] = QStringLiteral("None");
      jdoc.setObject(jobj);
      return jdoc;
    case sAction::Update:
      jobj["Action"] = QStringLiteral("Update");
      break;
    case sAction::Rollback:
      jobj["Action"] = QStringLiteral("Rollback");
      break;
  }

  switch (stg) {
    case StrategyType::compulsory:
      jobj["Strategy"] = QStringLiteral("Compulsory");
      break;
    case StrategyType::optional:
      jobj["Strategy"] = QStringLiteral("Optional");
      break;
    default:
      jobj["Strategy"] = QStringLiteral("Error");
      break;
  }

  jobj[kResponseField] = QStringLiteral("Yes");
  jobj["From"] = from.toString();
  jobj["Destination"] = dest->toString();
  jdoc.setObject(jobj);
  return jdoc;
}

template <typename VersionType>
::std::optional<RequestResponse<VersionType>> ParseConfirm(
    const QByteArray& raw) {
  return ParseResponse<VersionType>(raw);
}

template <typename VersionType>
QJsonDocument MakeConfirm(const RequestResponse<VersionType>& response) {
  //
  QJsonDocument jdoc;
  QJsonObject jobj;
  auto [act, stg, from, dest] = response;
  switch (act) {
    case sAction::None:
      jobj["Action"] = QStringLiteral("None");
      jdoc.setObject(jobj);
      return jdoc;
    case sAction::Update:
      jobj["Action"] = QStringLiteral("Update");
      break;
    case sAction::Rollback:
      jobj["Action"] = QStringLiteral("Rollback");
      break;
  }

  switch (stg) {
    case StrategyType::compulsory:
      jobj["Strategy"] = QStringLiteral("Compulsory");
      break;
    case StrategyType::optional:
      jobj["Strategy"] = QStringLiteral("Optional");
      break;
    default:
      jobj["Strategy"] = QStringLiteral("Error");
      break;
  }

  jobj["From"] = from.toString();
  jobj["Destination"] = dest.toString();
  jobj[kConfirmField] = QStringLiteral("Yes");
  jdoc.setObject(jobj);
  return jdoc;
}

template <typename VersionType>
JsonType Tell(const QByteArray& raw) {
  //
  QJsonObject jobj = QJsonDocument::fromJson(raw).object();
  if (jobj.contains(kRequestField)) return JsonType::Request;
  if (jobj.contains(kResponseField)) return JsonType::Response;
  if (jobj.contains(kConfirmField)) return JsonType::Confirm;

  return JsonType::Error;
}

}  // namespace otalib

#endif  // UPDATE_STRATEGY_HPP
