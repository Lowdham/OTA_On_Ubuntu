#ifndef UTILS_HPP
#define UTILS_HPP

#include <QAtomicInt>
#include <iostream>
#include <type_traits>

namespace otalib {

#define OTALIB_ENABLE_IF(...) \
  template <typename ::std::enable_if<+bool(__VA_ARGS__)>::type* = nullptr>

#define OTALIB_ENABLE_IF_T(...) \
  typename ::std::enable_if<+bool(__VA_ARGS__)>::type* = nullptr

#define OTALIB_DISABLE_IF(...) \
  template <typename ::std::enable_if<!bool(__VA_ARGS__)>::type* = nullptr>

#define OTALIB_DISABLE_IF_T(...) \
  typename ::std::enable_if<!bool(__VA_ARGS__)>::type* = nullptr

#define OTALIB_VARIANT_CONSTRUCTOR(Classname, StorageType, XType, xval) \
  template <typename XType,                                             \
            OTALIB_ENABLE_IF_T(                                         \
                ::std::is_constructible_v<StorageType, XType&&>)>       \
  Classname(XType&& xval) noexcept(                                     \
      ::std::is_nothrow_constructible_v<StorageType, XType&&>)

#define OTALIB_VARIANT_ASSIGNMENT(Classname, StorageType, XType, xval)         \
  template <typename XType,                                                    \
            OTALIB_ENABLE_IF_T(::std::is_assignable_v<StorageType&, XType&&>)> \
  Classname& operator=(XType&& xval) noexcept(                                 \
      ::std::is_nothrow_assignable_v<StorageType&, XType&&>)

class SpinLock : private QAtomicInt {
 public:
  class Acquire {
   public:
    Acquire(SpinLock& spinLock) : m_spinLock(spinLock) { m_spinLock.lock(); }

    ~Acquire() { m_spinLock.unlock(); }

   private:
    SpinLock& m_spinLock;

    // Disable copy constructor and assignment operator
    Acquire& operator=(const Acquire&);
    Acquire(const Acquire&);
  };

  SpinLock() : QAtomicInt(Unlocked) {}

  void lock() {
    while (!testAndSetOrdered(Unlocked, Locked))
      ;  // Busy-wait
  }

  void unlock() {
    while (!testAndSetOrdered(Locked, Unlocked))
      ;  // Busy-wait
  }

  bool tryLock() { return testAndSetOrdered(Unlocked, Locked); }

 private:
  static const int Unlocked = 1;
  static const int Locked = 0;
};

}  // namespace otalib

#endif  // UTILS_HPP
