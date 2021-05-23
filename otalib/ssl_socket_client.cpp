#include "ssl_socket_client.hpp"
using namespace otalib;

SSLSocketClient::SSLSocketClient(const QString &ip, quint16 port)
    : closed_(false) {
  initSSLSocket();

  ::memset(&si_, 0, sizeof(si_));
  si_.sin_family = AF_INET;
  si_.sin_addr.s_addr = ::inet_addr(ip.toStdString().c_str());
  si_.sin_port = ::htons(port);

  ssl_ = ::SSL_new(ctx_);
  ::SSL_set_fd(ssl_, sockfd_);
  ::SSL_set_connect_state(ssl_);
}

SSLSocketClient::~SSLSocketClient() {
  if (!closed_) this->close();
}

void SSLSocketClient::initSSLSocket() {
  sockfd_ = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  ctx_ = ::SSL_CTX_new(TLS_client_method());
}

bool SSLSocketClient::connectServer(quint32 timeout_second, int *error) {
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
    return false;
  } else if (ret == 0) {
    *error = errno;
    return false;
  } else if (FD_ISSET(sockfd_, &wset)) {
    ::fcntl(sockfd_, F_SETFL, flag);
    ::SSL_connect(ssl_);
    *error = ::SSL_get_error(ssl_, ret);
    return true;
  }
  return false;
}

bool SSLSocketClient::send(const char *data, size_t size) {
  sender_.append(data, size);
  size_t len = sender_.readable();
  int r_len = len;
  int n = -1;
  while (true) {
    n = ::SSL_write(ssl_, sender_.peek() + len - r_len, r_len);

    if (n < 0) {
      return false;
    }
    sender_.retired(n);
    r_len -= n;
    if (r_len <= 0) {
      sender_.retired_all();
      break;
    }
  }
  return true;
}

bool SSLSocketClient::send(const std::string &data) {
  return send(data.data(), data.size());
}

bool SSLSocketClient::send(const QByteArray &data) {
  return send(data.toStdString());
}

void SSLSocketClient::recv() {
  char buffer[kMaxBuffer];
  int n;
  DataSize totalSize = 0;
  while ((n = ::SSL_read(ssl_, buffer, kMaxBuffer))) {
    if (progressCb_) {
      totalSize += n;
      progressCb_(buffer, totalSize, n);
    }
    recver_.append(buffer, n);
  }
}

void SSLSocketClient::close() {
  if (closed_) return;
  ::close(sockfd_);
  ::SSL_shutdown(ssl_);
  ::SSL_free(ssl_);
  ::SSL_CTX_free(ctx_);
  closed_ = true;
}
