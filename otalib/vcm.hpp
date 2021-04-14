#ifndef VERSIONCONTROLMAP_HPP
#define VERSIONCONTROLMAP_HPP

#include <string.h>

#include <cmath>
#include <cstdint>
#include <functional>
#include <queue>
#include <unordered_map>

#include "version.hpp"

namespace otalib {

using VerDiff = int32_t;
using VerDist = int32_t;
using VerIndex = int32_t;
using LevelType = int16_t;

// It denote the max level that skiplist can build.
constexpr LevelType kVcmMaxLevel = 8;

// It denote the distance in the level 1
constexpr VerDist kVcmBasicDistance = 3;

// It denote the n value in log(n)
constexpr uint16_t kVcmFactor = 2;

enum class SearchStrategy { vUpdate, vRollback };

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
    lp.insert_or_assign(glver, back);

    // Append to stor;
    stor.push_back(glver);

    // Call the callback if it's not on initial construction.
    if (!onInitConstruct) NodeConstruct(back);
    return true;
  }

  // Thread-safe method for search the shortest path from "start" to "end".
  template <SearchStrategy stg = SearchStrategy::vUpdate>
  std::vector<EdgeType> search(const VersionType& start,
                               const VersionType& end) const noexcept {
    std::vector<EdgeType> route_path;
    if (lp.count(start) == 0 || lp.count(end) == 0) return route_path;

    VerIndex startIndex = lp.at(start);
    VerIndex goalIndex = lp.at(end);

    std::vector<VerIndex> vs;
    CalculateUnit<stg> unit(stor.size() + 10,
                            this);  // The extra "10" is as a buffer.
    unit.dfs(vs, route_path, startIndex, 1, goalIndex);

    return route_path;
  }

 private:
  template <SearchStrategy stg = SearchStrategy::vUpdate>
  struct CalculateUnit {
    VerIndex curMinHops_;
    bool* vis_;
    const VersionMap<VersionType>* parent_;

    CalculateUnit(VerIndex min_hop, const VersionMap<VersionType>* p)
        : curMinHops_(min_hop), vis_(new bool[min_hop]), parent_(p) {
      memset(vis_, 0, min_hop);
    }

    ~CalculateUnit() { delete[] vis_; }

    inline void dfs(std::vector<VerIndex>& indexs, std::vector<EdgeType>& edges,
                    VerIndex i, VerDist d, VerIndex goal) {
      if (i == goal) {
        VerIndex hops = indexs.size(), k;
        if (hops == 0) return;
        if (hops < curMinHops_) {
          curMinHops_ = hops;
          edges.clear();
          for (k = 0; k + 1 < hops; ++k) {
            edges.emplace_back(std::make_pair(indexs[k], indexs[k + 1]));
          }
          if (k == hops - 1) {
            edges.emplace_back(std::make_pair(indexs[k], goal));
          }
        }
        return;
      }

      if (curMinHops_ < (VerIndex)indexs.size()) return;

      // max level
      LevelType level =
          parent_->levelOfDistance(parent_->distanceOfVerIndex(i, goal));

      for (LevelType l = level; l >= 0; l--) {
        // the index is in level
        if (parent_->checkHit(l, i)) {
          d = parent_->distanceOfLevel(l);
          break;
        }
      }

      if constexpr (stg == SearchStrategy::vUpdate) {
        // ignored...
        double p = (double)rand() / RAND_MAX;
        if (p < 0.8) d = -d;
        // d = -d;
      }
      // rollback/update
      if (parent_->checkVerIndex(i - d) && !vis_[i - d]) {
        indexs.emplace_back(i);
        vis_[i] = true;
        dfs(indexs, edges, i - d, d, goal);
        vis_[i] = false;
        indexs.pop_back();
      }
      // update/rollback
      if (parent_->checkVerIndex(i + d) && !vis_[i + d]) {
        indexs.emplace_back(i);
        vis_[i] = true;
        dfs(indexs, edges, i + d, d, goal);
        vis_[i] = false;
        indexs.pop_back();
      }
    }
  };

 private:
  inline constexpr bool checkVerIndex(VerIndex index) const noexcept {
    return index >= 0 && index < (VerIndex)stor.size();
  }

  // Select an appropriate level from the current index difference
  inline constexpr LevelType levelOfDistance(VerDist dist) const noexcept {
    LevelType level = 0;
    if (dist >= 0 && dist < kVcmBasicDistance) return level;
    VerIndex l = kVcmBasicDistance;
    while (dist >= l) {
      level++;
      l <<= 1;
    }
    return level;
  }

  // Get the distance of two index.
  inline constexpr VerDist distanceOfVerIndex(VerIndex i,
                                              VerIndex j) const noexcept {
    return i < j ? j - i : i - j;
  }

  // Get the distance value on a certain level.
  inline constexpr VerDist distanceOfLevel(LevelType level) const noexcept {
    if (level == 0) return 1;

    VerDist distance = kVcmBasicDistance;
    LevelType max = level < kVcmMaxLevel ? level : kVcmMaxLevel;
    for (LevelType i = 1; i < max; ++i) distance *= kVcmFactor;
    return distance;
  }

  // Check whether the node is on a certain level.
  inline constexpr bool checkHit(uint16_t level,
                                 uint16_t index) const noexcept {
    return index % distanceOfLevel(level) == 0;
  }

  void NodeConstruct(VerIndex index) {
    //
    for (LevelType level = 0; level <= kVcmMaxLevel; ++level) {
      if (!checkHit(level, index)) continue;

      VerDist distance = distanceOfLevel(level);
      VerIndex prev = index - distance;
      if (prev < 0) continue;

      callback_on_ac(stor[prev], stor[index]);
    }
  }
};

}  // namespace otalib

#endif  // VERSIONCONTROLMAP_HPP
