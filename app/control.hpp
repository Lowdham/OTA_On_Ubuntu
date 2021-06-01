#ifndef CONTROL_HPP
#define CONTROL_HPP

#include "net_module.hpp"
#include "update_module.hpp"

namespace otalib::app {
//

class AppControl {
  //
  UpdateModule update_;

 public:
  AppControl() noexcept;

  void TryUpdate();

 private:
};
}  // namespace otalib::app

#endif  // CONTROL_HPP
