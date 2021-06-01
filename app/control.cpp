#include "control.hpp"

namespace otalib::app {

AppControl::AppControl() noexcept : update_() {
  //
}

void AppControl::TryUpdate() { update_.TryUpdate(); }

}  // namespace otalib::app
