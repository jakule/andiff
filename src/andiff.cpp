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
    __builtin_prefetch(static_cast<const void *>(&SA[mid]), 0, 1);

    const uint8_t *old_start = source.data() + SA[mid];
    T cmp_min = std::min(oldsize - SA[mid], newsize);
    T i = std::min(lmin, rmin);

    for (; i < cmp_min; ++i) {
      __builtin_prefetch(static_cast<const void *>(&target[i]), 0, 1);
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

struct diff_meta {
  int64_t ctrl_data;
  int64_t diff_data;
  int64_t extra_data;
  int64_t last_scan;
  int64_t last_pos;
};

template <typename _type, typename _derived, typename _writer>
class andiff_base {
 public:
  andiff_base(const std::vector<uint8_t> &source,
              const std::vector<uint8_t> &target, _writer writer,
              uint32_t threads_number)
      : SA(source.size() + 1),
        m_meta_data(threads_number),
        m_source(source),
        m_target(target),
        m_writer(writer),
        m_threads_number(threads_number) {}

  void run() {
    prepare();
    uint32_t threads_number = m_threads_number;
    std::vector<std::thread> threads(threads_number);
    std::thread save_thread(std::bind(&andiff_base::save, this));
    std::cout << "Comparison has been started using " << threads_number
              << " threads\n";

    _type range = (get_target_size() + 1) / threads_number;
    _type start = 0;

    for (uint32_t i = 0; i < threads_number; ++i) {
      _type end = start + range;
      threads[i] =
          std::thread(&andiff_base::diff, this, std::ref(m_meta_data[i]), start,
                      std::min(static_cast<_type>(m_target.size()), end));
      start = end;
    }

    for (uint32_t i = 0; i < threads_number; ++i) {
      threads[i].join();
    }

    save_thread.join();
  }

 protected:
  inline _type get_target_size() const {
    return static_cast<_type>(m_target.size());
  }

 private:
  void prepare() {
    int sa_result = generate_suffix_array<_type>(
        m_source.data(), SA.data(), static_cast<_type>(m_source.size()));
    enforce(sa_result == 0, "Generating sufix array failed");

    static_cast<_derived *>(this)->prepare_specific();
  }

  int diff(synchronized_queue<diff_meta> &meta_data, _type start, _type end) {
    _type scan, pos, len;
    _type lastscan, lastpos, lastoffset;
    _type oldscore, scsc;

    bool break_flag = false;  ///@todo I hate this flag....

    const _type tsize = get_target_size();
    const _type ssize = static_cast<_type>(m_source.size());

    scan = start;
    len = 0;
    pos = 0;
    lastscan = start;
    lastpos = 0;
    lastoffset = 0;
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
      };

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
          };
        };

        _type lenb = 0;
        if (scan < tsize) {
          _type s = 0;
          _type Sb = 0;
          for (_type i = 1; (scan >= lastscan + i) && (pos >= i); i++) {
            if (m_source[pos - i] == m_target[scan - i]) s++;
            if (s * 2 - i > Sb * 2 - lenb) {
              Sb = s;
              lenb = i;
            };
          };
        };

        if (lastscan + lenf > scan - lenb) {
          _type overlap = (lastscan + lenf) - (scan - lenb);
          _type s = 0;
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
            };
          };

          lenf += lens - overlap;
          lenb -= lens;
        };

        diff_meta dm = {lenf, (scan - lenb) - (lastscan + lenf),
                        (pos - lenb) - (lastpos + lenf), lastscan, lastpos};
        meta_data.push(dm);

        lastoffset = pos - scan;
        lastscan = scan - lenb;
        lastpos = pos - lenb;

        if (lastscan > end) {
          ///@todo: It works, but I don't like it
          if (break_flag) break;
          break_flag = true;
        }
      }
    }

    meta_data.close();
  }

  int save() {
    std::array<uint8_t, 8 * 3> buf;
    // Allocate size of output size or 16MB
    const int64_t block_size =
        std::min(m_target.size() + 1, 16UL * 1024 * 1024);
    /// @todo Fixed size array can be used
    std::vector<uint8_t> buffer(block_size);
    diff_meta dm = {};
    int64_t next_position = 0;

    for (auto &meta_data : m_meta_data) {
      while (meta_data.wait_and_pop(dm)) {
        if (dm.last_scan != next_position) continue;
        offtout(dm.ctrl_data, buf.data());
        offtout(dm.diff_data, buf.data() + 8);
        offtout(dm.extra_data, buf.data() + 16);

        // Write control data
        m_writer.write(buf.data(), buf.size());

        int64_t already_written_diff = 0;
        while (already_written_diff != dm.ctrl_data) {
          int64_t to_write =
              std::min(dm.ctrl_data - already_written_diff, block_size);
          // Write diff data
          for (int64_t i = 0; i < to_write; i++)
            buffer[i] = m_target[dm.last_scan + already_written_diff + i] -
                        m_source[dm.last_pos + already_written_diff + i];

          m_writer.write(buffer.data(), to_write);
          already_written_diff += to_write;
        }

        int64_t already_written_data = 0;
        while (already_written_data != dm.diff_data) {
          int64_t to_write =
              std::min(dm.diff_data - already_written_data, block_size);
          // Write extra data
          for (int64_t i = 0; i < to_write; i++)
            buffer[i] = m_target[dm.last_scan + dm.ctrl_data +
                                 already_written_data + i];
          m_writer.write(buffer.data(), to_write);
          already_written_data += to_write;
        }

        next_position = dm.ctrl_data + dm.diff_data + dm.last_scan;
      }
    }

    enforce(next_position == static_cast<_type>(m_target.size()),
            "Multithreading failed....");
    return 0;
  }

 protected:
  std::vector<_type> SA;
  std::vector<synchronized_queue<diff_meta>> m_meta_data;
  const std::vector<uint8_t> &m_source;
  const std::vector<uint8_t> &m_target;
  _writer m_writer;
  const uint32_t m_threads_number;
  int32_t __aligment_fix;
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
                _writer &writer)
      : base(source, target, writer, threads_number) {}

  void prepare_specific() {
    for (typename std::vector<_type>::size_type i = m_source[SA[0]];
         i < m_source.size() - 1; ++i) {
      if (m_source[SA[i]] != m_source[SA[i + 1]]) {
        dict_array[m_source[SA[i + 1]]] = i;
      }
    }
  }

  inline _type get_letter_range_end(_type new_first_letter) const {
    return new_first_letter != 255
               ? dict_array[new_first_letter + 1] - dict_array[new_first_letter]
               : static_cast<_type>(m_source.size()) - dict_array[255];
  }

  inline _type search(_type scan, _type &pos) const {
    uint8_t new_first_letter = m_target[scan];
    return search_simple(SA, m_source, &m_target[scan],
                         get_target_size() - scan, &pos,
                         dict_array[new_first_letter],
                         this->get_letter_range_end(new_first_letter));
  }

 private:
  _type dict_array[256] = {0};
};

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

int main(int argc, char *argv[]) {
  try {
    if (argc < 4) {
      std::cerr << "Usage: " << argv[0] << " oldfile newfile patchfile\n"
                << std::endl;
      exit(1);
    }

    bool is_lcp = false;

    /// @todo This is stupid, but in some cases may even works
    if (argc >= 5 && std::string(argv[4]) == "--lcp") {
      is_lcp = true;
    }

    ssize_t source_size, target_size;

    file_reader source_file;
    source_file.open(argv[1]);
    source_size = source_file.size();
    std::vector<uint8_t> source(source_size);
    source_file.read(source.data(), source_size);
    source_file.close();

    file_reader target_file;
    target_file.open(argv[2]);
    target_size = target_file.size();
    std::vector<uint8_t> target(target_size);
    target_file.read(target.data(), target_size);
    target_file.close();

    andiff_writer aw;
    aw.open(argv[3]);

    // Save magic
    aw.write_magic(andiff_magic, target_size);
    aw.open_bz_stream();

    // Use int32_t for all structures when both files are smaller than 2GB.
    // This can save a lot of memory and also speed up computation a bit.
    if (source_size < std::numeric_limits<int32_t>::max() &&
        target_size < std::numeric_limits<int32_t>::max()) {
      if (is_lcp) {
        std::cout << "32 lcp" << std::endl;
        andiff_runner<andiff_lcp, int32_t>(source, target, aw);
      } else {
        std::cout << "32" << std::endl;
        andiff_runner<andiff_simple, int32_t>(source, target, aw);
      }
    } else {
      /// @todo add lcp support
      std::cout << "64" << std::endl;
      andiff_runner<andiff_simple, int64_t>(source, target, aw);
    }
    aw.close();  // If exception has been thrown output file won't be closed,
                 // but this is not a big problem because OS will do that

  } catch (std::bad_alloc &e) {
    std::cerr << "Cannot allocate memory: " << e.what() << std::endl;
    return 2;
  } catch (std::exception &e) {
    std::cerr << e.what() << "std\n";
    return 3;
  } catch (...) {
    std::cerr << "Something went wrong\n";
    return 4;
  }

  return 0;
}
