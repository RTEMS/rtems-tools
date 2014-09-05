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
    /*
     * Defintion of flags to be filtered.
     */
    enum flag_type
    {
      ft_cppflags = 1 << 0,
      ft_cflags   = 1 << 1,
      ft_cxxflags = 1 << 2,
      ft_ldflags  = 1 << 3
    };

    extern std::string cc;             //< The CC executable as absolute path.
    extern std::string cc_name;        //< The CC name, ie gcc, clang.
    extern std::string exec_prefix;    //< The CC executable prefix.

    extern std::string cppflags;       //< The CPP flags.
    extern std::string cflags;         //< The CC flags.
    extern std::string cxxflags;       //< The CXX flags.
    extern std::string ldflags;        //< The LD flags.

    extern std::string warning_cflags; //< The warning flags in cflags.
    extern std::string include_cflags; //< The include flags in cflags.
    extern std::string machine_cflags; //< The machine flags in cflags.
    extern std::string spec_cflags;    //< The spec flags in cflags.

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
     * Strip the flags of -O and -g options.
     *
     * @param flags The flags a space delimited list to strip.
     * @return const std::string The stripped flags.
     */
    const std::string strip_cflags (const std::string& flags);

    /**
     * Filter the flags. Provide the type of flags being passed, the flags as a
     * space separated list, the architure, and a path. Provide strings
     * containers for the different flag groups so they can be sorted and
     * returned.
     *
     * @param flags The flags a space delimited list to strip.
     * @param arch The architecure per the OS specific name.
     * @param path A path to adjust based on the architecture.
     * @param type The type of flags being passed.
     * @param warnings Return warning flags in this string.
     * @param includes Return include flags in this string.
     * @param machines Return machine flags in this string.
     * @param specs Return spec flags in this string.
     * @return const std::string The filtered flags.
     */
    const std::string filter_flags (const std::string& flags,
                                    const std::string& arch,
                                    const std::string& path,
                                    flag_type          type,
                                    std::string&       warnings,
                                    std::string&       includes,
                                    std::string&       machines,
                                    std::string&       specs);

    /**
     * Filter the cflags and update the warnings, includes, machines and specs
     * if the type of flags is cflags. Provide the cflags as a space separated
     * list, the architure, and a path.
     *
     * @param flags The flags a space delimited list to strip.
     * @param arch The architecure per the OS specific name.
     * @param path A path to adjust based on the architecture.
     * @param type The type of flags being passed.
     * @return const std::string The filtered flags.
     */
    const std::string filter_flags (const std::string& flags,
                                    const std::string& arch,
                                    const std::string& path,
                                    flag_type          type);

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
