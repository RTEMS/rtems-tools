/*
 * Copyright (c) 2011-2014, Chris Johns <chrisj@rtems.org>
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
#include <rld-process.h>

namespace rld
{
  namespace cc
  {
    extern std::string cc;             //< The CC executable as absolute path.
    extern std::string cc_name;        //< The CC name, ie gcc, clang.
    extern std::string exec_prefix;    //< The CC executable prefix.

    extern std::string cppflags;       //< The CPP flags.
    extern std::string cflags;         //< The CC flags.
    extern std::string cxxflags;       //< The CXX flags.
    extern std::string ldflags;        //< The LD flags.

    extern std::string install_path;   //< The CC reported install path.
    extern std::string programs_path;  //< The CC reported programs path.
    extern std::string libraries_path; //< The CC reported libraries path.

    /**
     * Make a CC command from the set arguments.
     */
    void make_cc_command (rld::process::arg_container& args);

    /**
     * If the cppflags has been set append to the arguments.
     */
    void add_cppflags (rld::process::arg_container& args);

    /**
     * If the cflags has been set append to the arguments.
     */
    void add_cflags (rld::process::arg_container& args);

    /**
     * If the cxxflags has been set append to the arguments.
     */
    void add_cxxflags (rld::process::arg_container& args);

    /**
     * If the ldflags has been set append to the arguments.
     */
    void add_ldflags (rld::process::arg_container& args);

    /**
     * Get the standard libraries paths from the compiler.
     */
    void get_standard_libpaths (rld::path::paths& libpaths);

    /**
     * Get the standard libraries. Optionally add the C++ library.
     */
    void get_standard_libs (rld::path::paths& libs,
                            rld::path::paths& libpaths,
                            bool              cpp = false);

  }
}

#endif
