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

enum class SearchStrategy { vUpdate, vRollback };

template <typename VersionType>
class VersionMap {
  using VerDiff = typename VersionType::VerDiff;
  using VerDist = typename VersionType::VerDist;
  using VerIndex = typename VersionType::VerIndex;
  using LevelType = typename VersionType::LevelType;

  // Lookup should have O(1) in lookup.
  // Storage should have O(1) in access.
  // Storage has its own meta-capacity. Calculated by function
  // "CapcityOfVcm(...)" in "version.hpp".
  using Lookup =
      ::std::unordered_map<VersionType, VerIndex, typename VersionType::hasher>;
  using Storage = ::std::vector<VersionType>;
  using Callback = bool(const VersionType&, const VersionType&);

  Lookup lp;
  Storage stor;

  // Callback called when new node appends.
  ::std::function<Callback> callback_on_ac;

  // VersionMap attribute.
  static constexpr uint8_t vcm_max_level = VersionType::vcm_max_level;
  static constexpr uint8_t vcm_basic_distance = VersionType::vcm_basic_distance;
  static constexpr uint8_t vcm_factor = VersionType::vcm_factor;
  static constexpr uint64_t vcm_capacity = VersionType::vcm_capacity;

  static constexpr bool inherit_check =
      ::std::is_base_of_v<Version<VersionType, VCM::Large>, VersionType> ||
      ::std::is_base_of_v<Version<VersionType, VCM::Mid>, VersionType> ||
      ::std::is_base_of_v<Version<VersionType, VCM::Tiny>, VersionType>;

  static_assert(inherit_check,
                "The customized \"VersionType\" should implement the interface "
                "\"Version\" in file \"version.hpp\".");

 public:
  using EdgeType = ::std::pair<VersionType, VersionType>;

 public:
  VersionMap()
      : lp(VersionType::vcm_capacity, VersionType::hasher()),
        stor(),
        callback_on_ac() {}

  template <typename Function>
  void setCallback(Function&& f) noexcept {
    static_assert(::std::is_same_v<Function, Callback>,
                  "Type of function doesn't match the callback.");
    callback_on_ac = std::forward<Function>(f);
  }

  bool append(const VersionType& glver, bool onInitConstruct = false) {
    // Safe check.
    if (!stor.empty() && glver <= stor.back()) return false;

    // Capacity check.
    if (stor.size() == vcm_capacity) return false;

    // Build a index.
    VerIndex index = stor.size();
    lp.insert_or_assign(glver, index);

    // Append to stor;
    stor.push_back(glver);

    // Call the callback if it's not on initial construction.
    if (!onInitConstruct) NodeConstruct(index);
    return true;
  }

  // Thread-safe method for search the shortest path from "start" to "end".
  template <SearchStrategy stg = SearchStrategy::vUpdate>
  std::vector<EdgeType> search(const VersionType& start,
                               const VersionType& end) const noexcept {
    std::vector<EdgeType> route_path;
    if (lp.count(start) == 0 || lp.count(end) == 0) return route_path;
    if (start == end) return route_path;

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
            edges.emplace_back(parent_->stor[indexs[k]],
                               parent_->stor[indexs[k + 1]]);
          }
          if (k == hops - 1) {
            edges.emplace_back(parent_->stor[indexs[k]], parent_->stor[goal]);
          }
        }
        return;
      }

      // prune
      if (curMinHops_ < static_cast<VerIndex>(indexs.size())) return;

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
    return index >= 0 && index < static_cast<VerIndex>(stor.size());
  }

  // Select an appropriate level from the current index difference
  inline constexpr LevelType levelOfDistance(VerDist dist) const noexcept {
    LevelType level = 0;
    if (dist >= 0 && dist < vcm_basic_distance) return level;
    VerIndex l = vcm_basic_distance;
    while (dist >= l) {
      level++;
      l <<= 1;
    }
    return level;
  }

  // Get the distance of two index.
  inline constexpr VerDist distanceOfVerIndex(VerIndex i, VerIndex j) const
      noexcept {
    return i < j ? j - i : i - j;
  }

  // Get the distance value on a certain level.
  inline constexpr VerDist distanceOfLevel(LevelType level) const noexcept {
    if (level == 0) return 1;

    VerDist distance = vcm_basic_distance;
    LevelType max = level < vcm_max_level ? level : vcm_max_level;
    for (LevelType i = 1; i < max; ++i) distance *= vcm_factor;
    return distance;
  }

  // Check whether the node is on a certain level.
  inline constexpr bool checkHit(LevelType level, VerIndex index) const
      noexcept {
    return index % distanceOfLevel(level) == 0;
  }

  void NodeConstruct(VerIndex index) {
    //
    for (LevelType level = 0; level <= vcm_max_level; ++level) {
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
