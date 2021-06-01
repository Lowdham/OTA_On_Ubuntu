#ifndef OTASERVER_DIRECTORYWATCHER_H
#define OTASERVER_DIRECTORYWATCHER_H

#include <QString>
#include <filesystem>

#include "ThreadPool.h"
using namespace otaserver::net;
namespace fs = std::filesystem;

namespace otaserver {

constexpr static const size_t kDelayTime = 2000;

class DirectoryWatcher {
 public:
  using DirectoryChangedCb = std::function<void(const QString& verDir)>;
  enum DirAction { Init, Add };

  DirectoryWatcher() {}
  explicit DirectoryWatcher(const QString& dir) : dir_(dir) {}

  void startWatch(const QString& dir = "") {
    if (dir_.isEmpty()) dir_ = dir;

    ThreadPool<>::instance().add(std::bind(&DirectoryWatcher::listen, this));
  }

  void setDirChangedCb(const DirectoryChangedCb& cb) {
    directoryChangedCb_ = std::move(cb);
  }

 private:
  void listen() {
    while (true) {
      fs::directory_iterator dir(dir_.toStdString());
      for (auto& it : dir) {
        if (!it.is_directory()) continue;
        // add/remove directory
        std::string versionDir = it.path().filename();
        // add directory

        if (init_) {
          dirMp_[versionDir] = Init;
        } else {
          auto x = dirMp_.find(versionDir);
          if (x == dirMp_.end()) {
            if (directoryChangedCb_)
              directoryChangedCb_(QString::fromStdString(versionDir));
            dirMp_[versionDir] = Add;
          }
        }
      }
      init_ = false;
      // delay
      std::this_thread::sleep_for(std::chrono::milliseconds(kDelayTime));
    }
  }

 private:
  QString dir_;
  bool init_ = true;
  DirectoryChangedCb directoryChangedCb_;
  std::unordered_map<std::string, DirAction> dirMp_;
};

}  // namespace otaserver

#endif
