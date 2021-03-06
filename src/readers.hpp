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
#include <unistd.h>

#include <bzlib.h>

#if 0
/// Unused interface
class base_data_reader {
 public:
  virtual void open(const std::string& file_path) = 0;

  virtual ssize_t read(std::uint8_t * buf, ssize_t size) = 0;

  virtual bool eof() = 0;

  virtual ssize_t seek(ssize_t pos) = 0;

  virtual void close() = 0;
};
#endif

class file_reader {
 public:
  file_reader() : m_fd(-1), m_curr_pos(0), m_size(0) {}

  void open(const std::string& file_path) {
    m_fd = ::open(file_path.c_str(), O_RDONLY);
    enforce(m_fd > 0, "Cannot open file");
    m_size = get_file_size();
  }

  ssize_t size() { return m_size; }

  template <typename Type>
  ssize_t read(Type* buf, ssize_t size) {
    ssize_t chunk = ::read(m_fd, buf, size);
    enforce(chunk > 0, "Read 0 bytes");
    m_curr_pos += chunk;
    return static_cast<ssize_t>(chunk);
  }

  ssize_t seek(ssize_t pos) {
    ssize_t ret = ::lseek(m_fd, pos, SEEK_SET);
    enforce(ret == pos, "lseek error");
    m_curr_pos = ret;
    return ret;
  }

  void close() { ::close(m_fd); }

 private:
  ssize_t get_file_size() {
    ssize_t seek_ret = lseek(m_fd, 0, SEEK_END);
    enforce(seek_ret != -1, "lseek error");
    ssize_t file_size = seek_ret;
    ssize_t seek_ret_again = lseek(m_fd, 0, SEEK_SET);
    enforce(seek_ret_again == 0, "");
    return file_size;
  }

  int m_fd;
  ssize_t m_curr_pos;
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

  anpatch_reader(anpatch_reader&& an_reader) noexcept = default;

  template <size_t N>
  void open(const std::string& file_path, const char (&magic)[N]) {
    m_fd = std::fopen(file_path.c_str(), "r");
    enforce(m_fd, "Cannot open bz2 file");

    check_magic(magic);

    int bz2err;
    m_bz2file = BZ2_bzReadOpen(&bz2err, m_fd, 0, 0, NULL, 0);
    enforce(bz2err == BZ_OK, "bz2 read error");
  }

  ssize_t size() {
    ssize_t seek_ret = std::fseek(m_fd, 0, SEEK_END);
    enforce(seek_ret != -1, "bad seek");
    ssize_t file_size = std::ftell(m_fd);
    std::fseek(m_fd, 0, SEEK_SET);
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
