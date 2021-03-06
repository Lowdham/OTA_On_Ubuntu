#include "ssl_socket_client.hpp"
using namespace otalib;

//    #define SSL_ERROR_NONE 0
//    #define SSL_ERROR_SSL 1
//    #define SSL_ERROR_WANT_READ 2
//    #define SSL_ERROR_WANT_WRITE 3
//    #define SSL_ERROR_SYSCALL 5
//    #define SSL_ERROR_ZERO_RETURN 6

SSLSocketClient::SSLSocketClient(const QString &ip, quint16 port)
    : closed_(false) {
  ::signal(SIGPIPE, SIG_IGN);
  initSSLSocket();

  ::memset(&si_, 0, sizeof(si_));
  si_.sin_family = AF_INET;
  si_.sin_addr.s_addr = ::inet_addr(ip.toStdString().c_str());
  si_.sin_port = ::htons(port);
}

SSLSocketClient::~SSLSocketClient() {
  if (!closed_) close();
  ::SSL_free(ssl_);
  ::SSL_CTX_free(ctx_);
}

void SSLSocketClient::initSSLSocket() {
  ctx_ = ::SSL_CTX_new(TLS_client_method());
  ssl_ = ::SSL_new(ctx_);
}

bool SSLSocketClient::connectServer(quint32 timeout_second, int *error) {
  sockfd_ = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

  fd_set wset;
  FD_ZERO(&wset);
  FD_SET(sockfd_, &wset);
  struct timeval tv;
  tv.tv_sec = timeout_second;
  tv.tv_usec = 0;

  int flag = ::fcntl(sockfd_, F_GETFL);
  ::fcntl(sockfd_, F_SETFL, flag | O_NONBLOCK);

  if (-1 == ::connect(sockfd_, (struct sockaddr *)&si_, sizeof(si_))) {
    if (errno != EINPROGRESS) {
      *error = errno;
      ::close(sockfd_);
      return false;
    }
  }

  int ret = ::select(sockfd_ + 1, nullptr, &wset, nullptr, &tv);
  if (ret == -1) {
    *error = errno;
    ::close(sockfd_);
    return false;
  } else if (ret == 0) {
    *error = errno;
    ::close(sockfd_);
    return false;
  } else if (FD_ISSET(sockfd_, &wset)) {
    ::fcntl(sockfd_, F_SETFL, flag);

    ::SSL_set_fd(ssl_, sockfd_);
    ::SSL_set_connect_state(ssl_);
    ret = ::SSL_connect(ssl_);

    *error = ::SSL_get_error(ssl_, ret);
    if (*error == SSL_ERROR_SYSCALL) *error = errno;
    if (ret != 1) {
      ::close(sockfd_);
      ::SSL_shutdown(ssl_);
      return false;
    }

    return true;
  }
  return false;
}

///
/// \brief SSLSocketClient::send
/// \param data
/// \param size
/// \return error code 0:successed
///
int SSLSocketClient::send(const char *data, size_t size) {
  sender_.append(data, size);
  size_t len = sender_.readable();
  int r_len = len;
  int n = -1;
  while (true) {
    n = ::SSL_write(ssl_, sender_.peek() + len - r_len, r_len);

    if (n < 0) {
      return ::SSL_get_error(ssl_, n);
    }
    sender_.retired(n);
    r_len -= n;
    if (r_len <= 0) {
      sender_.retired_all();
      break;
    }
  }
  int err = ::SSL_get_error(ssl_, n);
  if (err == SSL_ERROR_SYSCALL) err = errno;
  return err;
}

int SSLSocketClient::send(const std::string &data) {
  return send(data.data(), data.size());
}

int SSLSocketClient::send(const QByteArray &data) {
  return send(data.toStdString());
}

///
/// \brief SSLSocketClient::recv
///
int SSLSocketClient::recv() {
  char buffer[kMaxBuffer];
  int n;
  DataSize totalSize = 0;
  while ((n = ::SSL_read(ssl_, buffer, kMaxBuffer)) > 0) {
    if (progressCb_) {
      totalSize += n;
      progressCb_(buffer, totalSize, n);
    }
    recver_.append(buffer, n);
  }
  int err = ::SSL_get_error(ssl_, n);
  // if (err == SSL_ERROR_SYSCALL) err = errno;
  return err;
}

void SSLSocketClient::close() {
  if (closed_) return;
  ::close(sockfd_);
  ::SSL_shutdown(ssl_);

  closed_ = true;
}
