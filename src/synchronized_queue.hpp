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

///
/// \brief Synchronized queue
///
/// Synchronized queue to ensure safe communication between threads. After
/// inserting all data synchronized_queue::close method should be called, to
/// close the queue and inform other threads that no more data will be added.
///
template <typename T>
class synchronized_queue {
 public:
  explicit synchronized_queue();

  ///
  /// \brief Destructor
  /// It will assert when queue is not empty
  ~synchronized_queue();

  ///
  /// \brief Push data into queue
  /// \param data Data
  ///
  void push(T& data);

  ///
  /// \brief Pop from queue.
  /// 		If data are not available wait.
  /// 		If queue has been closed and there is no more elements return
  ///     false.
  /// \param data Output data
  /// \return true if more data is available
  ///
  bool wait_and_pop(T& data);

  ///
  /// \brief Check if queue is closed
  /// \return true if queue is closed
  ///
  bool closed() const;

  ///
  /// \brief Close queue
  ///
  /// 	After calling this method no more data can be added to queue
  ///
  void close();

  ///
  /// \brief Get size of queue
  /// \return Size of queue
  ///
  size_t size() const;

  ///
  /// \brief Check is queue is empty
  /// \return true if empty
  ///
  bool empty() const;

 private:
  std::queue<T> m_data;          ///< Internal queue
  std::mutex m_mutex;            ///< Internal mutex
  std::condition_variable m_cv;  ///< Internal condition variable
  std::atomic_bool m_closed;     ///< Keeps state of
};

template <typename T>
synchronized_queue<T>::synchronized_queue() : m_data(), m_closed(false) {}

template <typename T>
synchronized_queue<T>::~synchronized_queue() {
  enforce(!size(), "destroying not empty queue");
}

template <typename T>
void synchronized_queue<T>::push(T& data) {
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_data.push(data);
  }
  m_cv.notify_one();
}

template <typename T>
bool synchronized_queue<T>::wait_and_pop(T& data) {
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

template <typename T>
bool synchronized_queue<T>::closed() const {
  return m_closed;
}

template <typename T>
void synchronized_queue<T>::close() {
  m_closed = true;
  m_cv.notify_one();
}

template <typename T>
size_t synchronized_queue<T>::size() const {
  return m_data.size();
}

template <typename T>
bool synchronized_queue<T>::empty() const {
  return size() == 0;
}

#endif  // SYNCHRONIZED_QUEUE_HPP
