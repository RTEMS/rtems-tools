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
#include <rld-rtems.h>

#include <pkgconfig.h>

namespace rld
{
  namespace rtems
  {
    static std::string _version = RTEMS_VERSION;
    static std::string _path;
    static std::string _arch_bsp;

    static void
    load_cc ()
    {
      path::paths parts;
      std::string rtems_pkgconfig;
      std::string bsp;

      if (_path.empty ())
        throw rld::error ("Not set", "RTEMS path");

      bsp = rtems_arch_bsp ();

      parts.push_back ("lib");
      parts.push_back ("pkgconfig");

      rld::path::path_join (path (), parts, rtems_pkgconfig);

      if (!path::check_directory (rtems_pkgconfig))
        throw rld::error ("Invalid RTEMS path", path ());

      rld::path::path_join (rtems_pkgconfig, bsp + ".pc", rtems_pkgconfig);

      if (!path::check_file (rtems_pkgconfig))
        throw rld::error ("RTEMS BSP not found", arch_bsp ());

      if (rld::verbose () >= RLD_VERBOSE_INFO)
        std::cout << " rtems: " << _arch_bsp << ": "
                  << rtems_pkgconfig << std::endl;

      pkgconfig::package pkg (rtems_pkgconfig);

      /*
       * Check the pc file matches what we ask for.
       */
      std::string name;
      if (!pkg.get ("name", name))
        throw rld::error ("RTEMS BSP no name in pkgconfig file", _arch_bsp);

      if (name != bsp)
        throw rld::error ("RTEMS BSP does not match the name in pkgconfig file",
                          _arch_bsp);

      std::string flags;

      if (pkg.get ("CPPFLAGS", flags))
      {
        rld::cc::append_flags (flags, arch (), path (), rld::cc::ft_cppflags);
        if (rld::verbose () >= RLD_VERBOSE_INFO)
          std::cout << " rtems: " << arch_bsp ()
                    << ": CPPFLAGS="
                    << rld::cc::get_flags (rld::cc::ft_cppflags)
                    << std::endl;
      }

      if (pkg.get ("CFLAGS", flags))
      {
        rld::cc::append_flags (flags, arch (), path (), rld::cc::ft_cflags);
        if (rld::verbose () >= RLD_VERBOSE_INFO)
        {
          std::cout << " rtems: " << arch_bsp ()
                    << ": CFLAGS=" << rld::cc::get_flags (rld::cc::ft_cflags)
                    << std::endl;
          std::cout << " rtems: " << _arch_bsp
                    << ": WARNINGS=" << rld::cc::get_flags (rld::cc::fg_warning_flags)
                    << std::endl;
          std::cout << " rtems: " << arch_bsp ()
                    << ": INCLUDES=" << rld::cc::get_flags (rld::cc::fg_include_flags)
                    << std::endl;
          std::cout << " rtems: " << arch_bsp ()
                    << ": MACHINES=" << rld::cc::get_flags (rld::cc::fg_machine_flags)
                    << std::endl;
          std::cout << " rtems: " << arch_bsp ()
                    << ": SPECS=" << rld::cc::get_flags (rld::cc::fg_spec_flags)
                    << std::endl;
        }
      }

      if (pkg.get ("CXXFLAGS", flags))
      {
        rld::cc::append_flags (flags, arch (), path (), rld::cc::ft_cxxflags);
        if (rld::verbose () >= RLD_VERBOSE_INFO)
          std::cout << " rtems: " << arch_bsp ()
                    << ": CXXFLAGS=" << rld::cc::get_flags (rld::cc::ft_cxxflags)
                    << std::endl;
      }

      if (pkg.get ("LDFLAGS", flags))
      {
        rld::cc::append_flags (flags, arch (), path (), rld::cc::ft_ldflags);
        if (rld::verbose () >= RLD_VERBOSE_INFO)
          std::cout << " rtems: " << arch_bsp ()
                    << ": LDFLAGS=" << rld::cc::get_flags (rld::cc::ft_ldflags)
                    << std::endl;
      }

      rld::cc::set_exec_prefix (arch ());
    }

    void
    set_version (const std::string& version_)
    {
      _version = version_;
    }

    void
    set_arch_bsp (const std::string& arch_bsp_)
    {
      _arch_bsp = arch_bsp_;
      if (!_path.empty ())
        load_cc ();
    }

    void
    set_path (const std::string& path_)
    {
      _path = path_;
      if (!_arch_bsp.empty ())
        load_cc ();
    }

    const std::string
    version ()
    {
      return _version;
    }

    const std::string
    arch_bsp ()
    {
      return _arch_bsp;
    }

    const std::string
    arch ()
    {
      if (_arch_bsp.empty ())
        throw rld::error ("No arch/bsp name", "rtems: arch");
      std::string::size_type slash = _arch_bsp.find_first_of ('/');
      if (slash == std::string::npos)
        throw rld::error ("Invalid BSP name", _arch_bsp);
      return _arch_bsp.substr (0, slash);
      std::string bsp  = _arch_bsp.substr (slash + 1);
    }

    const std::string
    bsp ()
    {
      if (_arch_bsp.empty ())
        throw rld::error ("No arch/bsp name", "rtems: bsp");
      std::string::size_type slash = _arch_bsp.find_first_of ('/');
      if (slash == std::string::npos)
        throw rld::error ("Invalid BSP name", _arch_bsp);
      return _arch_bsp.substr (slash + 1);
    }

    const std::string
    path ()
    {
      return _path;
    }

    const std::string
    rtems_arch_prefix ()
    {
      return arch () + "-rtems" + version ();
    }

    const std::string
    rtems_arch_bsp ()
    {
      return rtems_arch_prefix () + '-' + bsp ();
    }

  }
}
