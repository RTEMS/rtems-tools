/*
 * Copyright (c) 2011-2012, Chris Johns <chrisj@rtems.org>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
/**
 * @file
 *
 * @ingroup rtems-ld
 *
 * @brief Various calls to CC.
 *
 */

#if !defined (_RLD_CC_H_)
#define _RLD_CC_H_

#include <string>

#include <rld-files.h>

namespace rld
{
  namespace cc
  {
    extern std::string cc;             //< The CC executable.
    extern std::string exec_prefix;    //< The CC executable prefix.
    extern std::string march;          //< The CC machine architecture.
    extern std::string mcpu;           //< The CC machine CPU.

    extern std::string install_path;   //< The CC reported install path.
    extern std::string programs_path;  //< The CC reported programs path.
    extern std::string libraries_path; //< The CC reported libraries path.

    /**
     * Get the standard libraries paths from the compiler.
     */
    void get_standard_libpaths (rld::files::paths& libpaths);

    /**
     * Get the standard libraries. Optionally add the C++ library.
     */
    void get_standard_libs (rld::files::paths& libs,
                            rld::files::paths& libpaths,
                            bool               cpp = false);

  }
}

#endif
