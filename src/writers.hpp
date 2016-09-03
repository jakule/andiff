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

#include <cstdio>
#if defined(_MSC_VER)
typedef ptrdiff_t ssize_t;
#else
#include <unistd.h>
#endif

#include <bzlib.h>

class file_writer {
public:
    file_writer() : m_fd(nullptr), m_curr_pos(0){};
    file_writer(const file_writer& other) = delete;
    file_writer(file_writer&& other) noexcept
        : m_fd(other.m_fd)
        , m_curr_pos(other.m_curr_pos)
    {
        other.m_fd = nullptr;
        other.m_curr_pos = -1;
    }

    ~file_writer() { close(); }

    void open(const std::string& file_path) {
        m_fd = fopen(file_path.c_str(), "wb");
        enforce(m_fd != nullptr, "Cannot open file for write");
  }

    template <typename Type>
    ssize_t write(Type* buf, ssize_t size) {
        enforce(m_fd, "file_writer not opened, already closed or moved from");
        ssize_t chunk = fwrite(buf, 1, size, m_fd);
        enforce(chunk > 0, "Wrote 0 bytes");
        m_curr_pos += chunk;
        return chunk;
    }

    void close() {
        if (m_fd)
            fclose(m_fd);
        m_fd = nullptr;
    }

private:
    FILE* m_fd;
    ssize_t m_curr_pos;
};

class andiff_writer {
 public:
  andiff_writer() = default;
  andiff_writer(andiff_writer& a) = default;

  void open(const std::string& file_path) {
    m_fd = fopen(file_path.c_str(), "wb");
    enforce(m_fd > 0, "Cannot open file for write");
  }

  template <typename T, size_t Size>
  void write_magic(T (&magic)[Size], int64_t new_size) {
    constexpr size_t string_size = Size - 1;  // Remove null character
    static_assert((Size - 1) == 16, "Magic size is different");
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
    BZ2_bzWrite(&bz2err, bz2, const_cast<Type*>(buf), size);
    enforce(bz2err == BZ_OK, "Error while writing bz2 data");

    return size;
  }

  void close() {
    BZ2_bzWriteClose(&bz2err, bz2, 0, NULL, NULL);
    fclose(m_fd);
  }

 private:
  FILE* m_fd;
  BZFILE* bz2;
  int bz2err;
};
#endif  // WRITERS_HPP
