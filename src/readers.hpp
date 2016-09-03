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

#ifndef READERS_HPP
#define READERS_HPP

#include "enforce.hpp"

#include <algorithm>
#include <string>
#include <vector>

#include <fcntl.h>

#include <cstdio>
#if defined(_MSC_VER)
typedef ptrdiff_t ssize_t;
#else
#include <unistd.h>
#endif

#include <bzlib.h>

class file_reader {
 public:
  file_reader() : m_fd(nullptr), m_size(0) {}
  file_reader(const file_reader& other) = delete;
  file_reader(file_reader&& other) noexcept
      : m_fd(other.m_fd)
      , m_size(other.m_size)
  {
      other.m_fd = nullptr;
      other.m_size = -1;
  }

  ~file_reader() { close(); }

  void open(const std::string& file_path) {
    m_fd = fopen(file_path.c_str(), "rb+");
    enforce(m_fd != nullptr, "Cannot open file");
    m_size = get_file_size();
  }

  ssize_t size() { return m_size; }

  template <typename Type>
  ssize_t read(Type* buf, ssize_t size) {
    enforce(m_fd, "file_reader not opened, already closed or moved from");
    ssize_t chunk = fread(buf, 1, size, m_fd);
    enforce(chunk > 0, "Read 0 bytes");
    return static_cast<ssize_t>(chunk);
  }

  ssize_t seek(ssize_t pos) {
    int ret = fseek(m_fd, pos, SEEK_SET);
    enforce(ret == 0, "fseek error");
    return pos;
  }

  void close() {
    if (m_fd)
      fclose(m_fd);
    m_fd = nullptr;
  }

 private:
  ssize_t get_file_size() {
    int seek_ret = fseek(m_fd, 0, SEEK_END);
    enforce(seek_ret == 0, "fseek error");
    ssize_t file_size = ftell(m_fd);
    int seek_ret_again = fseek(m_fd, 0, SEEK_SET);
    enforce(seek_ret_again == 0, "fseek error");
    return file_size;
  }

  FILE* m_fd;
  ssize_t m_size;
};

class anpatch_reader {
 public:
  anpatch_reader() : m_eof(false) {}

  template <size_t N>
  anpatch_reader(const std::string& file_path, const char (&magic)[N])
      : m_eof(false) {
    open(file_path, magic);
  }

  anpatch_reader(anpatch_reader&& an_reader) noexcept
      : m_fd(an_reader.m_fd)
      , m_bz2file(an_reader.m_bz2file)
      , m_eof(an_reader.m_eof)
  {
      an_reader.m_fd = nullptr;
      an_reader.m_bz2file = nullptr;
  }

  template <size_t N>
  void open(const std::string& file_path, const char (&magic)[N]) {
    m_fd = fopen(file_path.c_str(), "rb");
    enforce(m_fd, "Cannot open bz2 file");

    check_magic(magic);

    int bz2err;
    m_bz2file = BZ2_bzReadOpen(&bz2err, m_fd, 0, 0, NULL, 0);
    enforce(bz2err == BZ_OK, "bz2 read error");
  }

  ssize_t size() {
    ssize_t seek_ret = fseek(m_fd, 0, SEEK_END);
    enforce(seek_ret != -1, "fseek error");
    ssize_t file_size = ftell(m_fd);
    int seek_ret_again = fseek(m_fd, 0, SEEK_SET);
    enforce(seek_ret_again == 0, "fseek error");
    return file_size;
  }

  template <typename Type>
  ssize_t read(Type* buf, ssize_t size) {
    int bz2err;
    int n = BZ2_bzRead(&bz2err, m_bz2file, buf, int(size));
    if (bz2err == BZ_STREAM_END)
      m_eof = true;
    else
      enforce(bz2err == BZ_OK, "bz2 read error");
    enforce(n > 0,
            "bz2 read no data");  ///@todo What in case when eof was reached??
    return static_cast<ssize_t>(n);
  }

  bool eof() { return m_eof; }

  void close() {
    int bz2err;
    BZ2_bzReadClose(&bz2err, m_bz2file);
    fclose(m_fd);
  }

 private:
  template <size_t N>
  inline void check_magic(const char (&magic_string)[N]) {
    static_assert(N > 0, "N cannot be less than 1");
    std::vector<char> magic(N - 1);
    size_t read = fread(magic.data(), 1, magic.size(), m_fd);
    enforce(read == magic.size(), "");
    enforce(std::equal(magic.begin(), magic.end(), magic_string),
            "Wrong magic");

    int64_t patch_size;
    read = fread(&patch_size, 1, sizeof(int64_t), m_fd);
    enforce(read == sizeof(int64_t), "read error");
    enforce(patch_size >= 0, "Corrupt patch\n");
  }

  FILE* m_fd;
  BZFILE* m_bz2file;
  bool m_eof;
};

#endif  // READERS_HPP
