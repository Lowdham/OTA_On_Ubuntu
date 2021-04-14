#ifndef VERSION_H
#define VERSION_H

#include <QString>
#include <QStringList>

#include "logger/logger.h"

namespace otalib {

constexpr bool ver_debug_mode = true;

template <typename DerivedClass>
struct Version {
  inline bool operator<(const Version<DerivedClass>& rhs) const noexcept {
    return DerivedClass::compare(static_cast<const DerivedClass&>(*this),
                                 static_cast<const DerivedClass&>(rhs)) == -1;
  }

  inline bool operator<=(const Version<DerivedClass>& rhs) const noexcept {
    return DerivedClass::compare(static_cast<const DerivedClass&>(*this),
                                 static_cast<const DerivedClass&>(rhs)) != 1;
  }

  inline bool operator>(const Version<DerivedClass>& rhs) const noexcept {
    return DerivedClass::compare(static_cast<const DerivedClass&>(*this),
                                 static_cast<const DerivedClass&>(rhs)) == 1;
  }

  inline bool operator>=(const Version<DerivedClass>& rhs) const noexcept {
    return DerivedClass::compare(static_cast<const DerivedClass&>(*this),
                                 static_cast<const DerivedClass&>(rhs)) != -1;
  }

  inline bool operator==(const Version<DerivedClass>& rhs) const noexcept {
    return DerivedClass::compare(static_cast<const DerivedClass&>(*this),
                                 static_cast<const DerivedClass&>(rhs)) == 0;
  }

  inline bool operator!=(const Version<DerivedClass>& rhs) const noexcept {
    return DerivedClass::compare(static_cast<const DerivedClass&>(*this),
                                 static_cast<const DerivedClass&>(rhs)) != 0;
  }

  inline decltype(auto) operator-(
      const Version<DerivedClass>& rhs) const noexcept {
    return DerivedClass::minus(static_cast<const DerivedClass&>(*this),
                               static_cast<const DerivedClass&>(rhs));
  }

  inline decltype(auto) operator+(
      const Version<DerivedClass>& rhs) const noexcept {
    return DerivedClass::add(static_cast<const DerivedClass&>(*this),
                             static_cast<const DerivedClass&>(rhs));
  }
};

class GeneralVersion : public Version<GeneralVersion> {
  int version[3];
  // Version Pattern
  // X.X.X
  bool assign(const QString& str) noexcept {
    QStringList vlist = str.split(".");
    if (vlist.size() != 3)
      return false;
    else
      for (int i = 0; i < 3; ++i) version[i] = vlist.at(i).toInt();
    return true;
  }

 public:
  GeneralVersion() : Version<GeneralVersion>(), version{0} {}
  GeneralVersion(const QString& str) : Version<GeneralVersion>() {
    assign(str);
  }
  GeneralVersion(const GeneralVersion& xver) : Version<GeneralVersion>() {
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
        for (int j = i - 1; j > 0; --j) {
          ans.version[j] -= 1;
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
    for (int i = 0; i < 3; ++i)
      ans.version[i] = lhs.version[i] + rhs.version[i];
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

using VersionDemo = GeneralVersion;

}  // namespace otalib

#endif  // VERSION_H
