#ifndef UPDATE_MODULE_HPP
#define UPDATE_MODULE_HPP

#include <QJsonDocument>
#include <thread>

#include "../otalib/pack_apply.hpp"
#include "../otalib/shell_cmd.hpp"
#include "../otalib/update_strategy.hpp"
#include "../otalib/version.hpp"
#include "app_error.hpp"
#include "net_module.hpp"
#include "properties.hpp"

namespace otalib::app {
//
class UpdateModule {
  NetModule net_;

 public:
  UpdateModule() noexcept;

  void TryUpdate();

 private:
  bool doUpdate(const RequestResponse<AppVersionType>& response);
};
}  // namespace otalib::app

#endif  // !UPDATE_MODULE_HPP
