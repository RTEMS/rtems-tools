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
     * The type of flags.
     */
    enum flag_type
    {
      ft_cppflags = 1 << 0,
      ft_cflags   = 1 << 1,
      ft_cxxflags = 1 << 2,
      ft_ldflags  = 1 << 3
    };

    /*
     * Flags groups.
     */
    enum flag_group
    {
      fg_warning_flags,
      fg_include_flags,
      fg_machine_flags,
      fg_spec_flags
    };

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
     * Set CC. The exec-prefix is ignored if this is set.
     */
    void set_cc (const std::string& cc);

    /**
     * Get the CC.
     */
    const std::string get_cc ();

    /**
     * Is the CC set ?
     */
    bool is_cc_set ();

    /**
     * Set LD. The exec-prefix is ignored if this is set.
     */
    void set_ld (const std::string& ld);

    /**
     * Get the LD.
     */
    const std::string get_ld ();

    /**
     * Is the LD set ?
     */
    bool is_ld_set ();

    /**
     * Set the exec-prefix. If CC is set the exec-prefix is ignored.
     */
    void set_exec_prefix (const std::string& exec_prefix);

    /**
     * Get the exec-prefix.
     */
    const std::string get_exec_prefix ();

    /**
     * Is exec-prefix set ?
     */
    bool is_exec_prefix_set ();

    /**
     * Set the flags with a specific arch and include path.
     */
    void set_flags (const std::string& flags,
                    const std::string& arch,
                    const std::string& path,
                    flag_type          type);

    /**
     * Set the flags.
     */
    void set_flags (const std::string& flags, flag_type type);

    /**
     * Append the flags with a specific arch and include path.
     */
    void append_flags (const std::string& flags,
                       const std::string& arch,
                       const std::string& path,
                       flag_type          type);

    /**
     * Append the flags.
     */
    void append_flags (const std::string& flags, flag_type type);

    /**
     * Get the flags.
     */
    const std::string get_flags (flag_type type);
    const std::string get_flags (flag_group group);

    /**
     * Append the flags if set.
     */
    void append_flags (flag_type type, rld::process::arg_container& args);

    /**
     * Make a CC command from the set arguments.
     */
    void make_cc_command (rld::process::arg_container& args);

    /**
     * Make a LD command from the set arguments.
     */
    void make_ld_command (rld::process::arg_container& args);

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
