#include "../include/TcpConnection.h"

using namespace otaserver::net;

void TcpConnection::initialize(int connfd) {
  sender_.retired_all();
  recver_.retired_all();

  connfd_ = connfd;
  disconnected_ = false;
}

int TcpConnection::read(int *err) {
  int n = -1;
#ifdef SUPPORT_SSL_LIB
  char buffer[kMaxBuffer];
  while ((n = ::SSL_read(ssl_, buffer, kMaxBuffer)) > 0) {
    recver_.append(buffer, n);
  }
  *err = ::SSL_get_error(ssl_, n);
#else
  while ((n = recver_.read_fd(connfd_, err)) > 0)
    ;
#endif
  return n;
}

int TcpConnection::write(int *err) {
  size_t len = sender_.readable();
  int r_len = len;
  int n = -1;
  while (true) {
#ifdef SUPPORT_SSL_LIB
    n = ::SSL_write(ssl_, sender_.peek() + len - r_len, r_len);

    if (n < 0) {
      *err = ::SSL_get_error(ssl_, n);
      return n;
    }
#else
    n = ::write(connfd_, sender_.peek() + len - r_len, r_len);

    if (n < 0) {
      *err = errno;
      return n;
    }
#endif
    sender_.retired(n);
    r_len -= n;
    // send all
    if (r_len <= 0) {
      if (sender_.readable() == 0 && sender_.mapped_size() &&
          sender_.use_mapped()) {
        sender_.use_mapped(false);
        n = 0;
        sender_.retired_all();
        sender_.unmapped();
        break;
      } else if (sender_.readable() == 0 && sender_.mapped_size() &&
                 !sender_.use_mapped()) {
        len = sender_.mapped_size();
        r_len = len;
        sender_.has_written(len);
        sender_.use_mapped(true);
      } else {
        n = 0;
        sender_.retired_all();
        break;
      }
    }
  }
  return n;
}
