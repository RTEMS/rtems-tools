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

#include <rld.h>
#include <rld-cc.h>

#include <pkgconfig.h>

namespace rld
{
  namespace rtems
  {
    std::string version = "4.11";
    std::string path;
    bool        installed;
    std::string arch_bsp;

    const std::string
    arch (const std::string& ab)
    {
      std::string::size_type slash = ab.find_first_of ('/');
      if (slash == std::string::npos)
        throw rld::error ("Invalid BSP name", ab);
      return ab.substr (0, slash);
      std::string bsp  = ab.substr (slash + 1);
    }

    const std::string
    bsp (const std::string& ab)
    {
      std::string::size_type slash = ab.find_first_of ('/');
      if (slash == std::string::npos)
        throw rld::error ("Invalid BSP name", ab);
      return ab.substr (slash + 1);
    }

    const std::string
    rtems_bsp (const std::string& ab)
    {
      const std::string a = arch (ab);
      const std::string b = bsp (ab);
      return a + "-rtems" + version + '-' + b;
    }

    void
    load_cc ()
    {
      path::paths parts;
      std::string rtems_pkgconfig;
      std::string bsp;

      if (path.empty ())
        throw rld::error ("Not set; see -r", "RTEMS path");

      bsp = rtems_bsp (arch_bsp);

      parts.push_back ("lib");
      parts.push_back ("pkgconfig");

      rld::path::path_join (path, parts, rtems_pkgconfig);

      if (!path::check_directory (rtems_pkgconfig))
        throw rld::error ("Invalid RTEMS path", path);

      rld::path::path_join (rtems_pkgconfig, bsp + ".pc", rtems_pkgconfig);

      if (!path::check_file (rtems_pkgconfig))
        throw rld::error ("RTEMS BSP not found", arch_bsp);

      if (rld::verbose () >= RLD_VERBOSE_INFO)
        std::cout << " rtems: " << arch_bsp << ": "
                  << rtems_pkgconfig << std::endl;

      pkgconfig::package pkg (rtems_pkgconfig);

      std::string flags;

      if (pkg.get ("CPPFLAGS", flags))
      {
        rld::cc::cppflags = rld::cc::filter_flags (flags,
                                                   arch_bsp,
                                                   path,
                                                   rld::cc::ft_cppflags);
        if (rld::verbose () >= RLD_VERBOSE_INFO)
          std::cout << " rtems: " << arch_bsp
                    << ": CPPFLAGS=" << rld::cc::cppflags << std::endl;
      }

      if (pkg.get ("CFLAGS", flags))
      {
        rld::cc::cflags = rld::cc::filter_flags (flags,
                                                 arch_bsp,
                                                 path,
                                                 rld::cc::ft_cflags);
        if (rld::verbose () >= RLD_VERBOSE_INFO)
        {
          std::cout << " rtems: " << arch_bsp
                    << ": CFLAGS=" << rld::cc::cflags << std::endl;
          std::cout << " rtems: " << arch_bsp
                    << ": WARNINGS=" << rld::cc::warning_cflags << std::endl;
          std::cout << " rtems: " << arch_bsp
                    << ": INCLUDES=" << rld::cc::include_cflags << std::endl;
          std::cout << " rtems: " << arch_bsp
                    << ": MACHINES=" << rld::cc::machine_cflags << std::endl;
          std::cout << " rtems: " << arch_bsp
                    << ": SPECS=" << rld::cc::spec_cflags << std::endl;
        }
      }

      if (pkg.get ("CXXFLAGS", flags))
      {
        rld::cc::cxxflags = rld::cc::filter_flags (flags,
                                                   arch_bsp,
                                                   path,
                                                   rld::cc::ft_cxxflags);
        if (rld::verbose () >= RLD_VERBOSE_INFO)
          std::cout << " rtems: " << arch_bsp
                    << ": CXXFLAGS=" << rld::cc::cxxflags << std::endl;
      }

      if (pkg.get ("LDFLAGS", flags))
      {
        rld::cc::ldflags = rld::cc::filter_flags (flags,
                                                  arch_bsp,
                                                  path,
                                                  rld::cc::ft_ldflags);
        if (rld::verbose () >= RLD_VERBOSE_INFO)
          std::cout << " rtems: " << arch_bsp
                    << ": LDFLAGS=" << rld::cc::ldflags << std::endl;
      }
    }
  }
}
