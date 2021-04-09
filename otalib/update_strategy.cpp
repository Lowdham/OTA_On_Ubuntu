#include "update_strategy.h"

namespace otalib {
namespace {
// Strategy Pattern
/*
[
  {
    "name" = "XXX"
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
NOTE: "optional" means the client can decide whether update/rollback or not.
      "compulsory" means the client must update/rollback.

Request Json Pattern
{
  "name" = "...",
  "version" = "...",
  "type" = "..."
}
*/
enum class Compare { lt, lte, gt, gte, eq, neq, error };

struct ClientInfo {
  QString name;
  QString version;
  QString type;
};

struct VersionConditionBlock {
  Compare cmp;
  QString value;
};

struct TypeConditionBlock {
  Compare assign;  // only allow "eq" and "neq".
  QString name;
};

struct ConditionBlock {
  std::optional<VersionConditionBlock> vcond;
  std::optional<TypeConditionBlock> tcond;
};

struct UpdateBlock {
  StrategyType stg;
  std::optional<ConditionBlock> cond;
  QString dest;
};

struct RollbackBlock {
  StrategyType stg;
  std::optional<ConditionBlock> cond;
  QString dest;
};

VersionConditionBlock receiveVersionCond(const QJsonObject& object) {
  VersionConditionBlock vcb;
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

  if (object.contains("value") && object.value("value").isString())
    vcb.value = object.value("value").toString();
  else
    vcb.value = QString();

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

ConditionBlock receiveConditionBlock(const QJsonObject& object) {
  ConditionBlock cond;

  if (object.contains("version") && object.value("version").isObject()) {
    cond.vcond = receiveVersionCond(object.value("version").toObject());
  } else
    cond.vcond = ::std::nullopt;

  if (object.contains("type") && object.value("type").isObject()) {
    cond.tcond = receiveTypeCond(object.value("type").toObject());
  } else
    cond.tcond = ::std::nullopt;

  return cond;
}

template <typename ActionBlock>
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
    ab.cond = receiveConditionBlock(object.value("condition").toObject());
  else
    ab.cond = ::std::nullopt;

  if (object.contains("dest") && object.value("dest").isString())
    ab.dest = object.value("dest").toString();
  else
    ab.dest = QString();

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

ClientInfo getClientInfo(const QJsonObject& cobj) {
  //
  ClientInfo ret;
  if (cobj.contains("name") && cobj.value("name").isString())
    ret.name = cobj.value("name").toString();
  else
    ret.name = QStringLiteral("ERROR");

  if (cobj.contains("version") && cobj.value("version").isString())
    ret.version = cobj.value("version").toString();
  else
    ret.version = QStringLiteral("ERROR");

  if (cobj.contains("type") && cobj.value("type").isString())
    ret.type = cobj.value("type").toString();
  else
    ret.type = QStringLiteral("ERROR");

  return ret;
}

int compareVersion(const QString& lhs, const QString& rhs) noexcept {
  // Lhs > Rhs return 1
  // Lhs == Rhs return 0
  // Lhs < Rhs return -1
  QStringList llist = lhs.split(".");
  QStringList rlist = rhs.split(".");
  for (int i = 0; i < llist.size(); ++i) {
    if (i == rlist.size()) return 1;  // lhs is greater than rhs.
    int result = QString::compare(llist.at(i), rlist.at(i));
    if (result > 0)
      return 1;  // lhs is greater than rhs.
    else if (result == 0)
      continue;
    else
      return -1;  // rhs is greater than lhs.
  }

  // Equal
  return 0;
}

bool checkCondition(const ConditionBlock& cond, const ClientInfo& cinfo) {
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
      return compareVersion(cinfo.version, vcond.value) == 0;
    else if (vcond.cmp == Compare::neq)
      return compareVersion(cinfo.version, vcond.value) != 0;
    else if (vcond.cmp == Compare::lt)
      return compareVersion(cinfo.version, vcond.value) == -1;
    else if (vcond.cmp == Compare::lte)
      return (compareVersion(cinfo.version, vcond.value) == -1 ||
              compareVersion(cinfo.version, vcond.value) == 0);
    else if (vcond.cmp == Compare::gt)
      return compareVersion(cinfo.version, vcond.value) == 1;
    else if (vcond.cmp == Compare::gte)
      return (compareVersion(cinfo.version, vcond.value) == 1 ||
              compareVersion(cinfo.version, vcond.value) == 0);
  }
  return true;
}

::std::optional<CheckInfo> applyStrategy(const QJsonObject& strategy,
                                         const ClientInfo& cinfo) {
  auto checkVaildity = [](const auto& block) {
    if (block.stg == StrategyType::error || block.dest.isEmpty())
      return false;
    else
      return true;
  };

  // Check update strategy first.
  if (strategy.contains("update") && strategy.value("update").isArray()) {
    auto update_list = strategy.value("update").toArray();
    for (auto iter : update_list) {
      UpdateBlock ud = receiveActionBlock<UpdateBlock>(iter.toObject());
      if (!ud.cond.has_value() ||
          (ud.cond.has_value() && checkCondition(ud.cond.value(), cinfo))) {
        if (checkVaildity(ud))
          return CheckInfo{StrategyAction::update, ud.stg, ud.dest};
        else
          return ::std::nullopt;
      }
    }
  }

  if (strategy.contains("rollback") && strategy.value("rollback").isArray()) {
    auto rollback_list = strategy.value("rollback").toArray();
    for (auto iter : rollback_list) {
      RollbackBlock rb = receiveActionBlock<RollbackBlock>(iter.toObject());
      if (!rb.cond.has_value() ||
          (rb.cond.has_value() && checkCondition(rb.cond.value(), cinfo))) {
        if (checkVaildity(rb))
          return CheckInfo{StrategyAction::rollback, rb.stg, rb.dest};
        else
          return ::std::nullopt;
      }
    }
  }

  // No matched action.
  return ::std::nullopt;
}

::std::optional<CheckInfo> matchStrategy(const ClientInfo& cinfo) {
  //
  const QJsonArray& strategy = getServerStrategy();
  for (auto iter : strategy) {
    if (iter.isObject()) {
      QJsonObject current = iter.toObject();
      if (current.contains("name") && current.value("name").isString() &&
          current.value("name").toString() == cinfo.name) {
        return applyStrategy(current, cinfo);  // Match succeed.
      } else
        continue;  // Match failed.
    } else {
      // error, just ignore.
      continue;
    }
  }

  // Match failed.
  return ::std::nullopt;
}

}  // namespace

::std::optional<CheckInfo> updateStrategyCheck(const QByteArray& raw) {
  //
  QJsonParseError err;
  QJsonDocument doc = QJsonDocument::fromJson(raw, &err);
  if (err.error == QJsonParseError::NoError && doc.isObject()) {
    //
    return matchStrategy(getClientInfo(doc.object()));
  } else {
    // Error
    return ::std::nullopt;
  }
}

}  // namespace otalib
