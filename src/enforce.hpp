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

#ifndef ENFORCE_HPP
#define ENFORCE_HPP

#include <assert.h>
#include <stdlib.h>

#define STR(x) #x

#if defined(_MSC_VER)
  #define FUNCTION_NAME __FUNCTION__
#else
  #define FUNCTION_NAME __PRETTY_FUNCTION__
#endif

#if !defined(NDEBUG)
#define enforce(cond, msg) \
  do {                     \
    assert((cond) && msg); \
  } while (0)
#else
#define enforce(cond, msg)                                                 \
  do {                                                                     \
    if (!(cond)) {                                                         \
      fprintf(stderr, "Error ocured: %s\n%s at %s:%d function: %s\n", msg, \
              STR(cond), __FILE__, __LINE__, FUNCTION_NAME);               \
      std::exit(1);                                                        \
    };                                                                     \
  } while (0)
#endif

#endif  // ENFORCE_HPP
