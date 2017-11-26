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

#include "andiff.hpp"

int main(int argc, char *argv[]) {
  try {
    /// @todo Missing cmd paring
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

    file_reader source_file;
    source_file.open(argv[1]);
    ssize_t source_size = source_file.size();
    std::vector<uint8_t> source(source_size);
    source_file.read(source.data(), source_size);
    source_file.close();

    file_reader target_file;
    target_file.open(argv[2]);
    ssize_t target_size = target_file.size();
    std::vector<uint8_t> target(target_size);
    target_file.read(target.data(), target_size);
    target_file.close();

    andiff_writer aw{};
    aw.open(argv[3]);

    // Save magic
    aw.write_magic(andiff_magic, target_size);
    aw.open_bz_stream();

    // Use int32_t for all structures when both files are smaller than 2GB.
    // This can save a lot of memory and also speed up computation a bit.
    if (source_size < std::numeric_limits<int32_t>::max() &&
        target_size < std::numeric_limits<int32_t>::max()) {
      if (is_lcp) {
        std::cout << "Using 32-bit indexes with LCP" << std::endl;
        andiff_runner<andiff_lcp, int32_t>(source, target, aw);
      } else {
        std::cout << "Using 32-bit indexes" << std::endl;
        andiff_runner<andiff_simple, int32_t>(source, target, aw);
      }
    } else {
      /// @todo add lcp support
      std::cout << "Using 64-bit indexes" << std::endl;
      andiff_runner<andiff_simple, int64_t>(source, target, aw);
    }
    aw.close();  // If exception has been thrown output file won't be closed,
                 // but this is not a big problem because OS will do that

  } catch (std::bad_alloc &e) {
    std::cerr << "Cannot allocate memory: " << e.what() << '\n';
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
