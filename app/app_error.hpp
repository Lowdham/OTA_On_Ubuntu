#ifndef APP_ERROR_HPP
#define APP_ERROR_HPP

#include <cstdint>
#include <exception>
#include <string>

namespace otalib::app {

class AppError : public ::std::exception {
  //
 public:
  enum Index : uint8_t {
    index_success = 0,
    index_network_unfunctional = 1,
    index_network_send_fail = 2,
    index_network_recv_fail = 3,
    index_update_response_corrupt = 4,
  };

 private:
  Index code_;
  ::std::string msg_;

  void ComposeMessage() {
    //
    msg_+= "\n";
    switch (code_) {
      case index_success:
        msg_ = "Success.";
        break;
      case index_network_unfunctional:
        msg_ = "Failed to connect to the server.";
        break;
      case index_network_send_fail:
        msg_ = "Failed to send data to the server.";
        break;
      case index_network_recv_fail:
        msg_ = "Failed to receive data from the server.";
        break;
      case index_update_response_corrupt:
        msg_ =
            "Failed to parse the response from the server, file might be "
            "corrupted.";
        break;
    }
  }

 public:
  AppError(Index xcode) noexcept : code_(xcode) { ComposeMessage(); }

  const char* what() const noexcept override { return msg_.c_str(); }

  Index Index() const noexcept { return code_; }
};

}  // namespace otalib::app

#endif  // APP_ERROR_HPP
