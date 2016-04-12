/*-
 * Copyright 2003-2005 Colin Percival
 * Copyright 2012 Matthew Endsley
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

#include "andiff_private.hpp"
#include "file_maped_array.hpp"
#include "readers.hpp"
#include "writers.hpp"

#include <iostream>

#include <bzlib.h>

static int64_t offtin(uint8_t* buf) {
  int64_t y;

  y = buf[7] & 0x7F;
  y <<= 8;
  y += buf[6];
  y <<= 8;
  y += buf[5];
  y <<= 8;
  y += buf[4];
  y <<= 8;
  y += buf[3];
  y <<= 8;
  y += buf[2];
  y <<= 8;
  y += buf[1];
  y <<= 8;
  y += buf[0];

  if (buf[7] & 0x80) y = -y;

  return y;
}

template <typename block_type>
class anpatcher {
 public:
  anpatcher(file_array&& old_file, anpatch_reader&& patch_file,
            std::string& output_file, ssize_t block_size)
      : m_data(new block_type[block_size]),
        m_block_size(block_size),
        m_old_pos(0),
        m_old_file(std::move(old_file)),
        m_patch_file(std::move(patch_file)) {
    m_new_file.open(output_file);
  }

  void run() {
    while (!m_patch_file.eof()) {
      read_control_data();
      apply_diff();
      apply_data();
    }
  }

 private:
  void read_control_data() {
    uint8_t buf[8];
    for (int i = 0; i <= 2; i++) {
      m_patch_file.read(buf, sizeof(buf));
      m_ctrl[i] = offtin(buf);
    }
  }

  void apply_diff() {
    int64_t read_size = 0;
    while (read_size < m_ctrl[0]) {
      int64_t to_read = read_size + m_block_size > m_ctrl[0]
                            ? m_ctrl[0] - read_size
                            : m_block_size;
      ssize_t cur_read_size = m_patch_file.read(m_data.get(), to_read);
      for (ssize_t i = 0; i < cur_read_size; ++i) {
        m_data[i] += m_old_file[m_old_pos + read_size + i];
      }
      m_new_file.write(m_data.get(), cur_read_size);
      read_size += cur_read_size;
    }
    m_old_pos += m_ctrl[0];
  }

  void apply_data() {
    ssize_t processed = 0;
    while (processed < m_ctrl[1]) {
      ssize_t to_read = processed + m_block_size > m_ctrl[1]
                            ? m_ctrl[1] - processed
                            : m_block_size;
      ssize_t cur_read_size = m_patch_file.read(m_data.get(), to_read);
      m_new_file.write(m_data.get(), cur_read_size);
      processed += cur_read_size;
    }

    m_old_pos += m_ctrl[2];
  }

 private:
  int64_t m_ctrl[3];
  std::unique_ptr<block_type[]> m_data;
  int64_t m_block_size;
  int64_t m_old_pos;
  file_array m_old_file;
  anpatch_reader m_patch_file;
  file_writer m_new_file;
};

int main(int argc, char* argv[]) {
  try {
    if (argc != 4) {
      std::cerr << "Usage: " << argv[0] << " oldfile newfile patchfile"
                << std::endl;
      exit(1);
    }

    file_array old_file(argv[1]);
    anpatch_reader patch_file(argv[3], andiff_magic);
    std::string new_file = argv[2];

    anpatcher<uint8_t> patcher(std::move(old_file), std::move(patch_file),
                               new_file, 5 * 1024);
    patcher.run();
  } catch (std::exception& e) {
    std::cerr << "Something went wrong: " << e.what() << std::endl;
    return 2;
  }

  return 0;
}
