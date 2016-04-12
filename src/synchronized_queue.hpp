/*-
 * Copyright 2016 Jakub Nyckowski
 * All rights reserved
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted providing that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef SYNCHRONIZED_QUEUE_HPP
#define SYNCHRONIZED_QUEUE_HPP

#include "enforce.hpp"

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <thread>

template <typename T>
class synchronized_queue {
 public:
  explicit synchronized_queue() : m_data(), m_closed(false) {}

  ~synchronized_queue() { enforce(!size(), "destroying not empty queue"); }

  void push(T& data) {
    {
      std::lock_guard<std::mutex> lock(m_mutex);
      m_data.push(data);
    }
    m_cv.notify_one();
  }

  bool wait_and_pop(T& data) {
    std::unique_lock<std::mutex> lock(m_mutex);

    if (!empty()) {
      data = m_data.front();
      m_data.pop();
      return true;
    }

    if (!closed()) {
      m_cv.wait(lock, [&] { return closed() || !empty(); });
      if (empty() && closed()) return false;
      data = m_data.front();
      m_data.pop();
      return true;
    }
    return false;
  }

  bool closed() const { return m_closed; }

  void close() {
    m_closed = true;
    m_cv.notify_one();
  }

  size_t size() const { return m_data.size(); }

  bool empty() const { return size() == 0; }

 private:
  std::queue<T> m_data;
  std::mutex m_mutex;
  std::condition_variable m_cv;
  std::atomic_bool m_closed;
};

#endif  // SYNCHRONIZED_QUEUE_HPP
