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

#ifndef ANDIFF_HPP
#define ANDIFF_HPP

#include "andiff_lcp.hpp"
#include "andiff_private.hpp"
#include "enforce.hpp"
#include "generate_sa.hpp"
#include "matchlen.hpp"
#include "readers.hpp"
#include "synchronized_queue.hpp"
#include "writers.hpp"

#include <cstdint>
#include <iostream>
#include <vector>

struct diff_meta {
  int64_t ctrl_data;
  int64_t diff_data;
  int64_t extra_data;
  int64_t last_scan;
  int64_t last_pos;
  int64_t last_offset;
  int64_t scan;
};

bool operator==(const diff_meta &a, const diff_meta &b) {
  return a.ctrl_data == b.ctrl_data && a.diff_data == b.diff_data &&
         a.extra_data == b.extra_data && a.last_pos == b.last_pos &&
         a.last_scan == b.last_scan && a.last_offset == b.last_offset &&
         a.scan == b.scan;
}

struct data_range {
  int64_t start;
  int64_t end;
};

struct data_package {
  synchronized_queue<diff_meta> *dmeta;
  data_range drange;
};

template <typename _type, typename _derived, typename _writer>
class andiff_base {
 public:
  ///
  /// \brief Base class responsible for generating patch
  /// \param source Old file
  /// \param target New file
  /// \param writer File writer which inherits from base_data_writer
  /// \param threads_number Numbers of threads used for computations
  ///
  andiff_base(const std::vector<uint8_t> &source,
              const std::vector<uint8_t> &target, _writer writer,
              uint32_t threads_number);
  ///
  /// \brief Main method, start comparison
  ///
  void run();

 protected:
  ///
  /// \brief Target size getter
  /// \return Size of new/target file
  ///
  inline _type get_target_size() const;

  ///
  /// \brief Source size getter
  /// \return Size of old/source file
  ///
  inline _type get_source_size() const;

 private:
  ///
  /// \brief Precompute data required to main comparison like suffix array
  ///
  void prepare();

  ///
  /// \brief Helper method for andiff_base::diff method
  /// \param dpackage Queue with ranges to compare
  ///
  void process(synchronized_queue<data_package> &dpackage);

  ///
  /// \brief Main diff method
  /// \param meta_data  Output queue for computed data
  /// \param start      Beginning of comparison
  /// \param end        End of comparison
  /// \param lastscan   Last scanned position
  /// \param lastpos    Last position
  /// \param lastoffset Last offset
  ///
  void diff(synchronized_queue<diff_meta> &meta_data, _type start, _type end,
            _type lastscan, _type lastpos = 0, _type lastoffset = 0);

  ///
  /// \brief Transform processed block and save to file
  /// \param save_buffer Helper buffer to transform data
  /// \param dm          Diff data to be saved
  /// \return Position of next processed block
  ///
  int64_t save_helper(std::vector<uint8_t> &save_buffer, const diff_meta &dm);

  void save(std::vector<synchronized_queue<diff_meta>> &meta_data);

 protected:
  std::vector<_type> SA;                 ///< Suffix array
  const std::vector<uint8_t> &m_source;  ///< Source/old file
  const std::vector<uint8_t> &m_target;  ///< Target/new file
  _writer m_writer;  ///< File writer. Right now only Bz2 writer is supported
  const uint32_t m_threads_number;  ///< Number of threads used for processing
};

template <typename _type, typename _writer>
class andiff_simple
    : public andiff_base<_type, andiff_simple<_type, _writer>, _writer> {
 public:
  using base = andiff_base<_type, andiff_simple<_type, _writer>, _writer>;
  using base::SA;
  using base::m_source;
  using base::m_target;
  using base::get_target_size;

  andiff_simple(const std::vector<uint8_t> &source,
                const std::vector<uint8_t> &target, uint32_t threads_number,
                _writer &writer);

  void prepare_specific();

  inline _type get_letter_range_end(_type new_first_letter) const;

  inline _type search(_type scan, _type &pos) const;

 private:
  _type dict_array[256] = {0};
};

/////////// Helper functions ///////////

///
/// \brief Simple version of binary search. It looks for common string in both
///        arrays.
/// \param SA      Suffix array calculated from source array
/// \param source  Array where we look for pattern
/// \param target  Array with pattern to find
/// \param newsize Max size of pattern to find
/// \param pos     Output position when find a match
/// \param start   Where to start search
/// \param end     End of source array
/// \return Length of common string in both arrays
///
template <typename T>
static T search_simple(const std::vector<T> &SA,
                       const std::vector<uint8_t> &source,
                       const uint8_t *target, T newsize, T *pos, T start,
                       T end) {
  T lpos = start;
  T rpos = start + end;
  T lmin = 1;
  T rmin = 1;
  T oldsize = static_cast<T>(source.size());

  while (rpos - lpos > 1) {
    T mid = lpos + (rpos - lpos) / 2;

    const uint8_t *old_start = source.data() + SA[mid];
    T cmp_min = std::min(oldsize - SA[mid], newsize);
    T i = std::min(lmin, rmin);

    for (; i < cmp_min; ++i) {
      if (old_start[i] < target[i]) {  // move right
        lpos = mid;
        lmin = i;
        break;
      } else if (old_start[i] > target[i]) {
        rpos = mid;
        rmin = i;
        break;
      }
    }

    if (i == cmp_min) {
      rpos = mid;
      rmin = i;
    }
  }

  T rlen =
      matchlen(source.data() + SA[rpos], oldsize - SA[rpos], target, newsize);
  T llen =
      matchlen(source.data() + SA[lpos], oldsize - SA[lpos], target, newsize);
  *pos = SA[rlen >= llen ? rpos : lpos];
  return std::max(rlen, llen);
}

///
/// \brief Convert int64_t to array of uint8_t
/// \param x Value to convert
/// \param buf Output buffer
///
static void offtout(int64_t x, uint8_t *buf) {
  int64_t y;

  if (x < 0)
    y = -x;
  else
    y = x;

  *reinterpret_cast<int64_t *>(buf) = y;

  if (x < 0) buf[7] |= 0x80;

  return;
}

////////// andiff_base implementation //////////

template <typename _type, typename _derived, typename _writer>
andiff_base<_type, _derived, _writer>::andiff_base(
    const std::vector<uint8_t> &source, const std::vector<uint8_t> &target,
    _writer writer, uint32_t threads_number)
    : SA(source.size() + 1),
      m_source(source),
      m_target(target),
      m_writer(writer),
      m_threads_number(threads_number) {}

template <typename _type, typename _derived, typename _writer>
void andiff_base<_type, _derived, _writer>::run() {
  prepare();
  uint32_t threads_number = m_threads_number;
  std::vector<std::thread> threads(threads_number);
  std::cout << "Comparison has been started using " << threads_number
            << " threads\n";

  const _type block_size = std::min<_type>(
      2 * 1024 * 1024, (get_target_size() + 1) / threads_number);
  uint64_t iterations = get_target_size() / block_size;
  std::vector<synchronized_queue<diff_meta>> meta_data(iterations);
  std::thread save_thread(
      std::bind(&andiff_base::save, this, std::ref(meta_data)));

  synchronized_queue<data_package> data_queue;
  _type range_start = 0;
  _type range_end = block_size;
  uint64_t iter = 0;

  for (; iter < iterations - 1; ++iter) {
    data_range dr = {range_start, range_end};
    data_package dp = {&(meta_data[iter]), dr};
    data_queue.push(dp);
    range_start = range_end;
    range_end += block_size;
  }
  data_range dr = {range_start, get_target_size()};
  data_package dp = {&(meta_data[iter]), dr};
  data_queue.push(dp);

  for (uint32_t i = 0; i < threads_number; ++i) {
    threads[i] = std::thread(&andiff_base::process, this, std::ref(data_queue));
  }
  data_queue.close();

  for (uint32_t i = 0; i < threads_number; ++i) {
    threads[i].join();
  }
  save_thread.join();
}

template <typename _type, typename _derived, typename _writer>
_type andiff_base<_type, _derived, _writer>::get_target_size() const {
  return static_cast<_type>(m_target.size());
}

template <typename _type, typename _derived, typename _writer>
_type andiff_base<_type, _derived, _writer>::get_source_size() const {
  return static_cast<_type>(m_source.size());
}

template <typename _type, typename _derived, typename _writer>
void andiff_base<_type, _derived, _writer>::prepare() {
  int sa_result = generate_suffix_array<_type>(
      m_source.data(), SA.data(), static_cast<_type>(m_source.size()));
  enforce(sa_result == 0, "Generating suffix array failed");

  static_cast<_derived *>(this)->prepare_specific();
}

template <typename _type, typename _derived, typename _writer>
void andiff_base<_type, _derived, _writer>::process(
    synchronized_queue<data_package> &dpackage) {
  data_package dp;
  while (dpackage.wait_and_pop(dp)) {
    andiff_base::diff(*(dp.dmeta), dp.drange.start, dp.drange.end,
                      dp.drange.start);
  }
}

template <typename _type, typename _derived, typename _writer>
void andiff_base<_type, _derived, _writer>::diff(
    synchronized_queue<diff_meta> &meta_data, _type start, _type end,
    _type lastscan, _type lastpos, _type lastoffset) {
  _type scan, pos, len;
  _type oldscore, scsc;

  const _type tsize = get_target_size();
  const _type ssize = get_source_size();

  scan = start;
  len = 0;
  pos = 0;

  while (scan < tsize) {
    oldscore = 0;

    for (scsc = scan += len; scan < tsize; ++scan) {
      len = static_cast<_derived *>(this)->search(scan, pos);

      for (; scsc < scan + len; scsc++)
        if ((scsc + lastoffset < ssize) &&
            (m_source[scsc + lastoffset] == m_target[scsc]))
          oldscore++;

      if (((len == oldscore) && (len != 0)) || (len > oldscore + 8)) break;

      if ((scan + lastoffset < ssize) &&
          (m_source[scan + lastoffset] == m_target[scan]))
        oldscore--;
    }

    if ((len != oldscore) || (scan == tsize)) {
      _type s = 0;
      _type Sf = 0;
      _type lenf = 0;
      for (_type i = 0; (lastscan + i < scan) && (lastpos + i < ssize);) {
        if (m_source[lastpos + i] == m_target[lastscan + i]) s++;
        i++;
        if (s * 2 - i > Sf * 2 - lenf) {
          Sf = s;
          lenf = i;
        }
      }

      _type lenb = 0;
      if (scan < tsize) {
        s = 0;
        _type Sb = 0;
        for (_type i = 1; (scan >= lastscan + i) && (pos >= i); i++) {
          if (m_source[pos - i] == m_target[scan - i]) s++;
          if (s * 2 - i > Sb * 2 - lenb) {
            Sb = s;
            lenb = i;
          }
        }
      }

      if (lastscan + lenf > scan - lenb) {
        _type overlap = (lastscan + lenf) - (scan - lenb);
        s = 0;
        _type Ss = 0;
        _type lens = 0;
        for (_type i = 0; i < overlap; i++) {
          if (m_target[lastscan + lenf - overlap + i] ==
              m_source[lastpos + lenf - overlap + i])
            s++;
          if (m_target[scan - lenb + i] == m_source[pos - lenb + i]) s--;
          if (s > Ss) {
            Ss = s;
            lens = i + 1;
          }
        }

        lenf += lens - overlap;
        lenb -= lens;
      }

      diff_meta dm = {lenf,
                      (scan - lenb) - (lastscan + lenf),
                      (pos - lenb) - (lastpos + lenf),
                      lastscan,
                      lastpos,
                      lastoffset,
                      scan};
      meta_data.push(dm);

      lastoffset = pos - scan;
      lastscan = scan - lenb;
      lastpos = pos - lenb;

      if (lastscan > end) {
        break;
      }
    }
  }

  meta_data.close();
}

template <typename _type, typename _derived, typename _writer>
int64_t andiff_base<_type, _derived, _writer>::save_helper(
    std::vector<uint8_t> &save_buffer, const diff_meta &dm) {
  std::array<uint8_t, 8 * 3> buf;
  offtout(dm.ctrl_data, buf.data());
  offtout(dm.diff_data, buf.data() + 8);
  offtout(dm.extra_data, buf.data() + 16);

  // Write control data
  m_writer.write(buf.data(), buf.size());

  int64_t already_written_diff = 0;
  while (already_written_diff != dm.ctrl_data) {
    int64_t to_write = std::min<int64_t>(dm.ctrl_data - already_written_diff,
                                         save_buffer.size());
    // Write diff data
    for (int64_t i = 0; i < to_write; i++)
      save_buffer[i] = m_target[dm.last_scan + already_written_diff + i] -
                       m_source[dm.last_pos + already_written_diff + i];

    m_writer.write(save_buffer.data(), to_write);
    already_written_diff += to_write;
  }

  int64_t already_written_data = 0;
  while (already_written_data != dm.diff_data) {
    int64_t to_write = std::min<int64_t>(dm.diff_data - already_written_data,
                                         save_buffer.size());
    // Write extra data
    for (int64_t i = 0; i < to_write; i++)
      save_buffer[i] =
          m_target[dm.last_scan + dm.ctrl_data + already_written_data + i];
    m_writer.write(save_buffer.data(), to_write);
    already_written_data += to_write;
  }

  int64_t next_position = dm.ctrl_data + dm.diff_data + dm.last_scan;
  return next_position;
}

template <typename _type, typename _derived, typename _writer>
void andiff_base<_type, _derived, _writer>::save(
    std::vector<synchronized_queue<diff_meta>> &meta_data) {
  // Allocate array of output size or 16MB
  // (I think that 16 is as good as 8 and 32 megs)
  const uint64_t block_size =
      std::min<uint64_t>(m_target.size() + 1, 16UL * 1024 * 1024);
  std::vector<uint8_t> save_buffer(block_size);
  diff_meta dm = {};
  int64_t next_position = 0;

  /// @todo What if meta_array has no elements??
  meta_data[0].wait_and_pop(dm);
  next_position = save_helper(save_buffer, dm);
  diff_meta dm_old = dm;

  for (auto &mdata : meta_data) {
    while (mdata.wait_and_pop(dm)) {
      // Check if next block match, if we already have it, skip
      if (dm.last_scan < next_position) continue;
      // If next is farther than we expected, fill the gap
      if (dm.last_scan > next_position) {
        synchronized_queue<diff_meta> sdm;
        // Generate few blocks with "trusted" data
        diff(sdm, dm_old.scan, dm.last_scan, dm_old.last_scan, dm_old.last_pos,
             dm_old.last_offset);
        diff_meta dm1 = {};
        while (sdm.wait_and_pop(dm1)) {
          if (dm1.last_scan < next_position) continue;
          next_position = save_helper(save_buffer, dm1);
          dm_old = dm1;
        }
      }
      // When previous block is the same as current go and save the rest of
      // queue
      if (dm_old == dm) break;
    }

    while (mdata.wait_and_pop(dm)) {
      next_position = save_helper(save_buffer, dm);
      dm_old = dm;
    }
  }

  enforce(next_position == static_cast<_type>(m_target.size()) ||
              next_position != 0,
          "Not full patch has been generated.");
}

////////// andiff_simple //////////

template <typename _type, typename _writer>
andiff_simple<_type, _writer>::andiff_simple(const std::vector<uint8_t> &source,
                                             const std::vector<uint8_t> &target,
                                             uint32_t threads_number,
                                             _writer &writer)
    : base(source, target, writer, threads_number) {}

template <typename _type, typename _writer>
void andiff_simple<_type, _writer>::prepare_specific() {
  for (uint32_t i = 1; i < 256; ++i) {
    auto ret = std::lower_bound(
        SA.begin(), SA.end(), i,
        [&](const long int &a, const _type &b) { return m_source[a] < b; });
    dict_array[i] = ret != SA.end() ? std::distance(SA.begin(), ret) - 1
                                    : dict_array[i - 1];
  }
}

template <typename _type, typename _writer>
_type andiff_simple<_type, _writer>::get_letter_range_end(
    _type new_first_letter) const {
  return new_first_letter != 255
             ? dict_array[new_first_letter + 1] - dict_array[new_first_letter]
             : static_cast<_type>(m_source.size()) - dict_array[255];
}

template <typename _type, typename _writer>
_type andiff_simple<_type, _writer>::search(_type scan, _type &pos) const {
  uint8_t new_first_letter = m_target[scan];
  return search_simple(SA, m_source, &m_target[scan], get_target_size() - scan,
                       &pos, dict_array[new_first_letter],
                       this->get_letter_range_end(new_first_letter));
}

template <typename _type, typename _writer>
class andiff_lcp
    : public andiff_base<_type, andiff_lcp<_type, _writer>, _writer> {
  using base = andiff_base<_type, andiff_lcp<_type, _writer>, _writer>;
  using base::SA;
  using base::m_source;
  using base::m_target;

 public:
  andiff_lcp(const std::vector<uint8_t> &source,
             const std::vector<uint8_t> &target, uint32_t threads_number,
             _writer &writer)
      : base(source, target, writer, threads_number) {}

  void prepare_specific() {
    m_lcp =
        kasai(m_source.data(), SA.data(), static_cast<_type>(m_source.size()));
    m_lcp_lr = calculate_lcp_lr(m_lcp);
  }

  _type search(_type scan, _type &pos) const {
    return search_lcp(SA.data(), m_source.data(),
                      static_cast<_type>(m_source.size()), &m_target[scan],
                      static_cast<_type>(m_target.size()) - scan, &pos,
                      m_lcp.data(), m_lcp_lr.data());
  }

 private:
  std::vector<_type> m_lcp;
  std::vector<_type> m_lcp_lr;
};

template <template <typename, typename> class diff_class, typename T>
void andiff_runner(const std::vector<uint8_t> &old,
                   const std::vector<uint8_t> &target, andiff_writer &stream) {
  uint32_t thread_number = std::thread::hardware_concurrency();
  if (!thread_number) {
    std::cerr
        << "Could not detect number of available processors. Using one thread"
        << std::endl;
    thread_number = 1;
  }

  diff_class<T, andiff_writer> data_compare(old, target, thread_number, stream);
  data_compare.run();
}

#endif  // ANDIFF_HPP
