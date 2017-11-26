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

#ifndef ANDIFF_LCP_H
#define ANDIFF_LCP_H

#include "matchlen.hpp"

#include <algorithm>
#include <cstdint>
#include <vector>

template <typename T>
T compare_pattern(T offset, const uint8_t *pattern, T pattern_size,
                  const uint8_t *source, T source_size, T start) {
  T i = offset;
  for (; i < pattern_size && start + i < source_size &&
         pattern[i] == source[i + start];
       ++i)
    ;
  return i;
}

template <typename T>
inline T lcp_offset(T lpos, T rpos, const T *lcp, const T *lcp_lr) {
  return rpos - lpos == 1 ? lcp[lpos] : lcp_lr[lpos + (rpos - lpos) / 2];
}

inline bool less_eq(int64_t lcp_offset, const uint8_t *pattern,
                    int64_t pattern_size, const uint8_t *source,
                    int64_t source_size, int64_t start) {
  if (lcp_offset == pattern_size) return true;
  return start + lcp_offset < source_size &&
         pattern[lcp_offset] < source[start + lcp_offset];
}

template <typename T>
static T search_lcp(const T *SA, const uint8_t *old, T old_size,
                    const uint8_t *pattern, T pattern_size, T *pos,
                    const T *lcp, const T *lcp_lr) {
  T lpos = 0;
  T rpos = old_size;
  T lcp_l = compare_pattern(static_cast<T>(0), pattern, pattern_size, old,
                            old_size, SA[0]);
  T lcp_r = compare_pattern(static_cast<T>(0), pattern, pattern_size, old,
                            old_size, SA[rpos - 1]);
  while (rpos - lpos > 1) {
    T mid = lpos + (rpos - lpos) / 2;
    T loffset = lcp_offset(lpos, mid, lcp, lcp_lr);
    T roffset = lcp_offset(mid, rpos, lcp, lcp_lr);

    if (loffset >= roffset) {
      if (lcp_l < loffset) {
        lpos = mid;
      } else if (lcp_l > loffset) {
        rpos = mid;
        lcp_r = loffset;
      } else {
        T offset = compare_pattern(loffset, pattern, pattern_size, old,
                                   old_size, SA[mid]);
        if (less_eq(offset, pattern, pattern_size, old, old_size, SA[mid])) {
          rpos = mid;
          lcp_r = offset;
        } else {
          lpos = mid;
          lcp_l = offset;
        }
      }
    } else {
      if (lcp_r < roffset) {
        rpos = mid;
      } else if (lcp_r > roffset) {
        lpos = mid;
        lcp_l = roffset;
      } else {
        T offset = compare_pattern(roffset, pattern, pattern_size, old,
                                   old_size, SA[mid]);
        if (less_eq(offset, pattern, pattern_size, old, old_size, SA[mid])) {
          rpos = mid;
          lcp_r = offset;
        } else {
          lpos = mid;
          lcp_l = offset;
        }
      }
    }
  }

  T rlen = matchlen(old + SA[rpos] + lcp_r, old_size - SA[rpos] - lcp_r,
                    pattern + lcp_r, pattern_size - lcp_r) +
           lcp_r;
  T llen = matchlen(old + SA[lpos] + lcp_l, old_size - SA[lpos] - lcp_l,
                    pattern + lcp_l, pattern_size - lcp_l) +
           lcp_l;
  *pos = SA[rlen >= llen ? rpos : lpos];
  return std::max(rlen, llen);
}

template <typename T>
std::vector<T> kasai(const uint8_t *s, T *sa, T n) {
  T k = 0;
  std::vector<T> lcp(n);
  std::vector<T> rank(n);

  for (int i = 0; i < n; i++) rank[sa[i]] = i;

  for (int i = 0; i < n; i++, k ? k-- : 0) {
    if (rank[i] == n - 1) {
      k = 0;
      continue;
    }
    T j = sa[rank[i] + 1];
    while (i + k < n && j + k < n && s[i + k] == s[j + k]) k++;
    lcp[rank[i]] = k;
  }

  return lcp;
}

template <typename T>
T calculate_lcp_lr_util(const std::vector<T> &lcp, std::vector<T> &lcp_lr,
                        T start, T end) {
  if (end - start == 1) {
    return lcp[start];
  }

  T mid = start + (end - start) / 2;
  T val = std::min(calculate_lcp_lr_util<T>(lcp, lcp_lr, start, mid),
                   calculate_lcp_lr_util<T>(lcp, lcp_lr, mid, end));
  lcp_lr[mid] = val;
  return val;
}

template <typename T>
std::vector<T> calculate_lcp_lr(const std::vector<T> &lcp) {
  const auto size = lcp.size();
  std::vector<T> lcp_rl(size);

  const auto mid = size / 2;

  lcp_rl[mid] =
      std::min(calculate_lcp_lr_util<T>(lcp, lcp_rl, static_cast<T>(0), mid),
               calculate_lcp_lr_util<T>(lcp, lcp_rl, mid, size));

  return lcp_rl;
}

#endif  // ANDIFF_LCP_H
