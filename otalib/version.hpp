#ifndef VERSION_H
#define VERSION_H

#include <QString>
#include <QStringList>

namespace otalib {

inline constexpr uint64_t CapacityOfVcm(uint8_t vcm_max_level,
                                        uint8_t vcm_basic_distance,
                                        uint8_t vcm_factor) noexcept {
  if (vcm_max_level == 0) return 1;

  uint64_t capacity = vcm_basic_distance;
  for (uint64_t i = 1; i < vcm_max_level; ++i) capacity *= vcm_factor;
  return capacity;
}

enum class VCM { Large, Mid, Tiny };

template <VCM vcm = VCM::Mid>
struct VcmControl {
  // Medium size
  using VerDiff = int32_t;
  using VerDist = int32_t;
  using VerIndex = uint32_t;
  using LevelType = uint32_t;

  // It denote the max level that skiplist can build.
  inline static constexpr uint8_t vcm_max_level = 9;

  // It denote the distance in the level 1
  // Kvcmbasicdistance should be kept as small as possible and should be a power
  // of 2 or 3.
  inline static constexpr uint8_t vcm_basic_distance = 3;

  // It must be 2 and can not change due to the lookup algorithm limits.
  inline static constexpr uint8_t vcm_factor = 2;

  inline static constexpr uint64_t vcm_capacity =
      CapacityOfVcm(vcm_max_level, vcm_basic_distance, vcm_factor);
};

template <>
struct VcmControl<VCM::Large> {
  // Large size
  using VerDiff = int32_t;
  using VerDist = int32_t;
  using VerIndex = uint32_t;
  using LevelType = uint32_t;

  inline static constexpr uint8_t vcm_max_level = 12;
  inline static constexpr uint8_t vcm_basic_distance = 4;
  inline static constexpr uint8_t vcm_factor = 2;  // mustn't change
  inline static constexpr uint64_t vcm_capacity =
      CapacityOfVcm(vcm_max_level, vcm_basic_distance, vcm_factor);
};

template <>
struct VcmControl<VCM::Tiny> {
  // Tiny size
  using VerDiff = int32_t;
  using VerDist = int32_t;
  using VerIndex = uint32_t;
  using LevelType = uint32_t;

  inline static constexpr uint8_t vcm_max_level = 6;
  inline static constexpr uint8_t vcm_basic_distance = 3;
  inline static constexpr uint8_t vcm_factor = 2;  // mustn't change
  inline static constexpr uint64_t vcm_capacity =
      CapacityOfVcm(vcm_max_level, vcm_basic_distance, vcm_factor);
};

/*/////////////////////////////////////////////////////////////////////////
   Vcm requires the customized verison class to inherit the "Version" and
   implement some functions.
    Necessary one:
    I:  int compare(const DerivedClass& lhs,const DerivedClass& rhs) const;
    ret: -1 : lhs < rhs
    ret: 0  : lhs == rhs
    ret" 1  : lhs > rhs

    Optional two:
    I:  add.
    II: minus.
///////////////////////////////////////////////////////////////////////////*/
template <typename DerivedClass, VCM vcm = VCM::Mid>
struct Version : VcmControl<vcm> {
  inline bool operator<(const Version<DerivedClass>& rhs) const {
    return DerivedClass::compare(static_cast<const DerivedClass&>(*this),
                                 static_cast<const DerivedClass&>(rhs)) == -1;
  }

  inline bool operator<=(const Version<DerivedClass>& rhs) const {
    return DerivedClass::compare(static_cast<const DerivedClass&>(*this),
                                 static_cast<const DerivedClass&>(rhs)) != 1;
  }

  inline bool operator>(const Version<DerivedClass>& rhs) const {
    return DerivedClass::compare(static_cast<const DerivedClass&>(*this),
                                 static_cast<const DerivedClass&>(rhs)) == 1;
  }

  inline bool operator>=(const Version<DerivedClass>& rhs) const {
    return DerivedClass::compare(static_cast<const DerivedClass&>(*this),
                                 static_cast<const DerivedClass&>(rhs)) != -1;
  }

  inline bool operator==(const Version<DerivedClass>& rhs) const {
    return DerivedClass::compare(static_cast<const DerivedClass&>(*this),
                                 static_cast<const DerivedClass&>(rhs)) == 0;
  }

  inline bool operator!=(const Version<DerivedClass>& rhs) const {
    return DerivedClass::compare(static_cast<const DerivedClass&>(*this),
                                 static_cast<const DerivedClass&>(rhs)) != 0;
  }

  inline QString toString() const {
    return static_cast<const DerivedClass*>(this)->toString();
  }

  struct hasher {
    inline size_t operator()(const DerivedClass& key) const {
      ::std::string source(reinterpret_cast<const char*>(&key),
                           sizeof(DerivedClass));
      return ::std::hash<::std::string>()(source);
    }
  };
};

class GeneralVersion : public Version<GeneralVersion, VCM::Mid> {
  int version[3];
  // Version Pattern
  // X.X.X
  // Only allow version[0] > 10
  bool assign(const QString& str) noexcept {
    QStringList vlist = str.split(".");
    if (vlist.size() != 3)
      return false;
    else
      for (int i = 0; i < 3; ++i) version[i] = vlist.at(i).toInt();
    return true;
  }

 public:
  GeneralVersion() : Version<GeneralVersion, VCM::Mid>(), version{0} {}
  GeneralVersion(const QString& str) : Version<GeneralVersion, VCM::Mid>() {
    assign(str);
  }
  GeneralVersion(const GeneralVersion& xver)
      : Version<GeneralVersion, VCM::Mid>() {
    version[0] = xver.version[0];
    version[1] = xver.version[1];
    version[2] = xver.version[2];
  }
  ~GeneralVersion() {}

 public:
  static int compare(const GeneralVersion& lhs,
                     const GeneralVersion& rhs) noexcept {
    // Lhs > Rhs return 1
    // Lhs == Rhs return 0
    // Lhs < Rhs return -1
    for (int i = 0; i < 3; ++i)
      if (lhs.version[i] < rhs.version[i])
        return -1;
      else if (lhs.version[i] > rhs.version[i])
        return 1;

    return 0;
  }

  // Generate the difference value.
  static GeneralVersion minus(const GeneralVersion& lhs,
                              const GeneralVersion& rhs) noexcept {
    GeneralVersion ans;
    for (int i = 0; i < 3; ++i) {
      ans.version[i] = lhs.version[i] - rhs.version[i];
      if (i != 0 && ans.version[i] < 0) {
        ans.version[i] += 10;
        for (int j = i - 1; j >= 0; --j) {
          ans.version[j] -= 1;
          if (j == 0) break;

          if (ans.version[j] >= 0)
            break;
          else {
            ans.version[j] = 9;
            continue;
          }
        }
      }
    }

    return ans;
  }

  //
  static GeneralVersion add(const GeneralVersion& lhs,
                            const GeneralVersion& rhs) noexcept {
    GeneralVersion ans;
    bool cf = false;
    for (int i = 2; i >= 0; --i) {
      ans.version[i] = lhs.version[i] + rhs.version[i];
      if (cf) {
        ++ans.version[i];
        ans.version[i + 1] -= 10;
      }
      cf = (ans.version[i] >= 10);
    }
    return ans;
  }

  QString toString() const noexcept {
    QString ans;
    for (int i = 0; i < 3; ++i) {
      ans += QString::number(version[i]) + ".";
    }
    return ans.left(ans.size() - 1);
  }

  friend inline void swap(GeneralVersion& lhs, GeneralVersion& rhs) noexcept {
    for (int i = 0; i < 3; ++i) {
      ::std::swap(lhs.version[i], rhs.version[i]);
    }
  }
};

}  // namespace otalib

#endif  // VERSION_H
