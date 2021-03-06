// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/openssl_util.h"

#include <openssl/err.h>
#include <openssl/ssl.h>

#include "base/lock.h"
#include "base/logging.h"
#include "base/scoped_vector.h"
#include "base/singleton.h"

namespace base {

namespace {

unsigned long CurrentThreadId() {
  return static_cast<unsigned long>(PlatformThread::CurrentId());
}

// Singleton for initializing and cleaning up the OpenSSL library.
class OpenSSLInitSingleton {
 public:
  static OpenSSLInitSingleton* Get() {
    // We allow the SSL environment to leak for multiple reasons:
    //   -  it is used from a non-joinable worker thread that is not stopped on
    //      shutdown, hence may still be using OpenSSL library after the AtExit
    //      runner has completed.
    //   -  There are other OpenSSL related singletons (e.g. the client socket
    //      context) who's cleanup depends on the global environment here, but
    //      we can't control the order the AtExit handlers will run in so
    //      allowing the global environment to leak at least ensures it is
    //      available for those other singletons to reliably cleanup.
    return Singleton<OpenSSLInitSingleton,
                     LeakySingletonTraits<OpenSSLInitSingleton> >::get();
  }
 private:
  friend struct DefaultSingletonTraits<OpenSSLInitSingleton>;
  OpenSSLInitSingleton() {
    SSL_load_error_strings();
    SSL_library_init();
    OpenSSL_add_all_algorithms();
    int num_locks = CRYPTO_num_locks();
    locks_.reserve(num_locks);
    for (int i = 0; i < num_locks; ++i)
      locks_.push_back(new Lock());
    CRYPTO_set_locking_callback(LockingCallback);
    CRYPTO_set_id_callback(CurrentThreadId);
  }

  ~OpenSSLInitSingleton() {
    CRYPTO_set_locking_callback(NULL);
    EVP_cleanup();
    ERR_free_strings();
  }

  static void LockingCallback(int mode, int n, const char* file, int line) {
    OpenSSLInitSingleton::Get()->OnLockingCallback(mode, n, file, line);
  }

  void OnLockingCallback(int mode, int n, const char* file, int line) {
    CHECK_LT(static_cast<size_t>(n), locks_.size());
    if (mode & CRYPTO_LOCK)
      locks_[n]->Acquire();
    else
      locks_[n]->Release();
  }

  // These locks are used and managed by OpenSSL via LockingCallback().
  ScopedVector<Lock> locks_;

  DISALLOW_COPY_AND_ASSIGN(OpenSSLInitSingleton);
};

}  // namespace

void EnsureOpenSSLInit() {
  (void)OpenSSLInitSingleton::Get();
}

void ClearOpenSSLERRStack(const tracked_objects::Location& location) {
  if (logging::DEBUG_MODE && VLOG_IS_ON(1)) {
    int error_num = ERR_get_error();
    if (error_num == 0)
      return;

    std::string message;
    location.Write(true, true, &message);
    DVLOG(1) << "OpenSSL ERR_get_error stack from " << message;
    char buf[140];
    do {
      ERR_error_string_n(error_num, buf, arraysize(buf));
      DVLOG(1) << "\t" << error_num << ": " << buf;
      error_num = ERR_get_error();
    } while (error_num != 0);
  } else {
    ERR_clear_error();
  }
}

}  // namespace base
