#ifndef VERSIONCONTROLMAP_HPP
#define VERSIONCONTROLMAP_HPP

#include <cstdint>
#include <functional>
#include <unordered_map>

#include "version.hpp"

namespace otalib {

using VerDiff = int16_t;
using VerDist = int16_t;
using VerIndex = int16_t;
using LevelType = uint16_t;

// It denote the max level that skiplist can build.
constexpr LevelType kVcmMaxLevel = 4;

// It denote the distance in the level 1
constexpr VerDist kVcmBasicDistance = 3;

// It denote the n value in log(n)
constexpr uint16_t kVcmFactor = 2;

template <typename VersionType>
class VersionMap {
  // Lookup should have O(1) in lookup.
  // Storage should have O(1) in access.
  using Lookup = ::std::unordered_map<VersionType, VerIndex>;
  using Storage = ::std::vector<VersionType>;

  using Callback = bool(const VersionType&, const VersionType&);
  //
  Lookup lp;
  Storage stor;
  // Callback called when new node appends.
  ::std::function<Callback> callback_on_ac;

 public:
  using EdgeType = ::std::pair<VersionType, VersionType>;

 public:
  VersionMap() : lp(), stor(), callback_on_ac() {}

  template <typename Function>
  void setCallback(Function&& f) {
    static_assert(::std::is_same_v<Function, Callback>,
                  "Type of function doesn't match the callback.");
    callback_on_ac = std::forward<Function>(f);
  }

  bool append(const VersionType& glver, bool onInitConstruct = false) {
    // Build a index.
    VerIndex back = stor.size();
    lp.insert(glver, back);

    // Append to stor;
    stor.push_back(glver);

    // Call the callback if it's not on initial construction.
    if (!onInitConstruct) NodeConstruct(back);
  }

  ::std::vector<EdgeType> search(const VersionType& start,
                                 const VersionType& end) const noexcept {
    if (lp.count(start) == 0 || lp.count(end) == 0)
      return ::std::vector<EdgeType>();

    // TODO
  }

 private:
  // Get the distance value on a certain level.
  inline constexpr VerDist distanceOfLevel(LevelType level) const noexcept {
    if (level == 0) return 1;

    VerDist distance = kVcmBasicDistance;
    LevelType max = level < kVcmMaxLevel ? level : kVcmMaxLevel;
    for (LevelType i = 1; i < max; ++i) distance *= kVcmFactor;
    return distance;
  }

  // Check whether the node is on a certain level.
  inline constexpr bool check(uint16_t level, uint16_t index) {
    return index % distanceOfLevel(level) == 0;
  }

  void NodeConstruct(VerIndex index) {
    //
    for (LevelType level = 0; level <= kVcmMaxLevel; ++level) {
      if (!check(level, index)) continue;

      VerDist distance = distanceOfLevel(level);
      VerIndex prev = index - distance;
      if (prev < 0) continue;

      callback_on_ac(stor[prev], stor[index]);
    }
  }
};

}  // namespace otalib

#endif  // VERSIONCONTROLMAP_HPP
