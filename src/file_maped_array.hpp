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

#ifndef FILE_MAPED_ARRAY_HPP
#define FILE_MAPED_ARRAY_HPP

#include "enforce.hpp"
#include "readers.hpp"
#include "writers.hpp"

#include <cstdint>
#include <memory>

///
/// \brief The file_mapped_array class
///
/// Class can read data from file using only a constant buffer.
/// Size of the buffer can be specified in constructor.
///
template <typename T, typename block_type>
class file_mapped_array {
 public:
  ///
  /// \brief Class constructor
  /// \param file_name Path to file
  /// \param buffer_size Size of buffer
  ///
  explicit file_mapped_array(const std::string& file_name,
                             size_t buffer_size = 1024);

  ///
  /// \brief Removed copy constructor
  ///
  file_mapped_array(file_mapped_array const& fma) = delete;

  ///
  /// \brief Removed copy assignment operator
  ///
  file_mapped_array& operator=(file_mapped_array const&) = delete;

  ///
  /// \brief Move constructor, noexcept
  /// \param fma file_mapped_array object
  ///
  file_mapped_array(file_mapped_array&& fma) noexcept;

  ///
  /// \brief Overload of [] operator. It can be used to read data from
  /// underlying file
  /// \param pos Position to read
  /// \return Data at given position
  ///
  block_type& operator[](size_t pos);

 private:
  void fill_data(size_t pos);

  std::unique_ptr<block_type[]> m_data;  ///< Internal data buffer
  size_t m_offset;     ///< Offset of read data from the beginning of file
  size_t m_cache_end;  ///< End of read cache
  const size_t m_buffer_size;  ///< Size of buffer
  T m_reader;                  ///< Source of data
};

template <typename T, typename block_type>
file_mapped_array<T, block_type>::file_mapped_array(
    const std::string& file_name, size_t buffer_size)
    : m_data(new block_type[buffer_size]),
      m_offset(0),
      m_cache_end(0),
      m_buffer_size(buffer_size) {
  m_reader.open(file_name);
  fill_data(m_offset);
}

template <typename T, typename block_type>
file_mapped_array<T, block_type>::file_mapped_array(
    file_mapped_array&& fma) noexcept
    : m_data(fma.m_data.release()),
      m_offset(fma.m_offset),
      m_cache_end(fma.m_cache_end),
      m_buffer_size(fma.m_buffer_size),
      m_reader(fma.m_reader) {}

template <typename T, typename block_type>
block_type& file_mapped_array<T, block_type>::operator[](size_t pos) {
  if (!(pos >= m_offset && pos < m_offset + m_buffer_size)) {
    fill_data(pos);
  }
  return m_data[pos - m_offset];
}

template <typename T, typename block_type>
void file_mapped_array<T, block_type>::fill_data(size_t pos) {
  m_reader.seek(pos);

  size_t read = m_reader.read(m_data.get(), m_buffer_size);
  enforce(read > 0, "read error");
  m_offset = pos;
  m_cache_end = m_offset + read;
}

using file_array = file_mapped_array<file_reader, std::uint8_t>;
using bz2_array = file_mapped_array<anpatch_reader, std::uint8_t>;

#endif  // FILE_MAPED_ARRAY_HPP
