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

#ifndef WRITERS_HPP
#define WRITERS_HPP

#include "enforce.hpp"

#include <memory>
#include <vector>

#include <fcntl.h>
#include <unistd.h>

#include <bzlib.h>


class file_writer {
 public:
  file_writer() = default;
  file_writer(const file_writer&) = delete;
  file_writer(file_writer&&) noexcept = default;

  void open(const std::string& file_path) {
    m_fd = ::open(file_path.c_str(), O_CREAT | O_WRONLY | O_TRUNC,
                  S_IRUSR | S_IWUSR);
    enforce(m_fd > 0, "Cannot open file for write");
  }

  template <typename Type>
  ssize_t write(Type* buf, ssize_t size) {
    ssize_t chunk = ::write(m_fd, buf, static_cast<size_t>(size));
    enforce(chunk > 0, "Read 0 bytes");
    m_curr_pos += chunk;
    return chunk;
  }

  void close() { ::close(m_fd); }

 private:
  int m_fd{-1};
  ssize_t m_curr_pos{0};
};

class andiff_writer {
 public:
  andiff_writer() = default;
  andiff_writer(andiff_writer& a) = delete;

  void open(const std::string& file_path) {
    m_fd = std::fopen(file_path.c_str(), "wb");
    enforce(m_fd != nullptr, "Cannot open file for write");
  }

  template <typename T, size_t Size>
  void write_magic(T (&magic)[Size], int64_t new_size) {
    constexpr size_t string_size = Size - 1;  // Remove null character
    static_assert(string_size == 16, "Magic size is different");
    static_assert(sizeof(new_size) == 8, "New file header has different size");
    enforce(fwrite(magic, string_size, 1, m_fd) == 1 &&
                fwrite(&new_size, sizeof(new_size), 1, m_fd) == 1,
            "Failed to write header");
  }

  void open_bz_stream() {
    bz2 = BZ2_bzWriteOpen(&bz2err, m_fd, 9, 0, 0);
    enforce(bz2, "Cannot open bz2 stream");
  }

  template <typename Type>
  ssize_t write(Type* buf, ssize_t size) {
    int bz2err;
    BZ2_bzWrite(&bz2err, bz2, buf, static_cast<int>(size));
    enforce(bz2err == BZ_OK, "Error while writing bz2 data");

    return size;
  }

  void close() {
    BZ2_bzWriteClose(&bz2err, bz2, 0, nullptr, nullptr);
    std::fclose(m_fd);
  }

 private:
  FILE* m_fd{nullptr};
  BZFILE* bz2{nullptr};
  int bz2err{-1};
};
#endif  // WRITERS_HPP
