#ifndef OTAERR_HPP
#define OTAERR_HPP

#include <QString>
#include <exception>
#include <variant>

#include "utils.hpp"

namespace otalib {

#define STRING_SOURCE_LOCATION \
  QString("[%1,line:%2]").arg(__FILE__).arg(__LINE__)

// This struct store the error infomation of all functions in the system.
class OTAError : public ::std::exception {
 public:
  struct S_file_open_fail {
    QString file_;
    QString extra_;
  };

  struct S_file_copy_fail {
    QString target_;
    QString dest_;
    QString extra_;
  };

  struct S_delta_log_invalid_line {
    QString line_;
  };

  struct S_delta_log_unexpected_end {
    QString line_;
  };

  struct S_delta_file_generate_fail {
    QString file_;
    QString extra_;
  };

  struct S_general {
    QString extra_;
  };

  struct S_apply_pack_unexpected_fail {
    QString packname_;
    QString extra_;
  };

  struct S_file_delete_fail {
    QString filename_;
    QString extra_;
  };

  struct S_file_write_fail {
    QString filename_;
    QString extra_;
  };

  enum Index : uint8_t {
    index_success = 0,
    index_file_open_fail = 1,
    index_file_copy_fail = 2,
    index_deltalog_invalid_line = 3,
    index_deltafile_generate_fail = 4,
    index_general = 5,
    index_apply_pack_unexpected_fail = 6,
    index_file_delete_fail = 7,
    index_file_write_fail = 8,
  };

 private:
  using StorageType = ::std::variant<::std::monostate,              // 0
                                     S_file_open_fail,              // 1
                                     S_file_copy_fail,              // 2
                                     S_delta_log_invalid_line,      // 3
                                     S_delta_file_generate_fail,    // 5
                                     S_general,                     // 6
                                     S_apply_pack_unexpected_fail,  // 7
                                     S_file_delete_fail,            // 8
                                     S_file_write_fail              // 9
                                     >;
  StorageType stor_;
  ::std::string msg_;
  bool composed_;

 public:
  OTALIB_VARIANT_CONSTRUCTOR(OTAError, stor_, XErrT, xerror)
      : stor_(::std::forward<XErrT>(xerror)), composed_(false) {
    compose();
  }

  OTALIB_VARIANT_ASSIGNMENT(OTAError, stor_, XErrT, xerror) {
    stor_ = ::std::forward<XErrT>(xerror);
    return *this;
  }

  ~OTAError() = default;

 private:
  inline Index index() const noexcept {
    return static_cast<enum Index>(stor_.index());
  }

  void compose() {
    //
    composed_ = true;
    QString msg;
    switch (this->index()) {
      case index_success: {
        msg = "Success.";
        break;
      }
      case index_file_open_fail: {
        const auto& altr = ::std::get<index_file_open_fail>(stor_);
        msg = "File[" + altr.file_ + "] open failed.";
        break;
      }
      case index_file_copy_fail: {
        const auto& altr = ::std::get<index_file_copy_fail>(stor_);
        msg = "File copy faild.[" + altr.target_ + "]--->[" + altr.dest_ + "]";
        break;
      }
      case index_deltalog_invalid_line: {
        const auto& altr = ::std::get<index_deltalog_invalid_line>(stor_);
        msg = "Invalid line in delta log. line--[" + altr.line_ + "]";
        break;
      }
      case index_deltafile_generate_fail: {
        const auto& altr = ::std::get<index_deltafile_generate_fail>(stor_);
        msg = "[" + altr.file_ + "]Delta file cannot generate." + altr.extra_;
        break;
      }
      case index_general: {
        const auto& altr = ::std::get<index_general>(stor_);
        msg = altr.extra_;
        break;
      }
      case index_apply_pack_unexpected_fail: {
        const auto& altr = ::std::get<index_apply_pack_unexpected_fail>(stor_);
        msg = "[" + altr.packname_ + "] cannot apply into app." + altr.extra_;
        break;
      }
    }
    msg_ = msg.toStdString();
  }

 public:
  const char* what() const noexcept override { return msg_.c_str(); }
};  // namespace otalib

}  // namespace otalib

#endif  // OTAERR_HPP
