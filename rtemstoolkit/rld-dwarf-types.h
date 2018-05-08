/*
 * Copyright (c) 2018, Chris Johns <chrisj@rtems.org>
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
 * @brief RTEMS Linker DWARF types.
  *
 */

#if !defined (_RLD_DWARF_TYPES_H_)
#define _RLD_DWARF_TYPES_H_

#include <dwarf.h>
#include <libdwarf.h>

namespace rld
{
  namespace dwarf
  {
    /**
     * Hide the types from libdwarf we use.
     */
    typedef ::Dwarf_Debug    dwarf;
    typedef ::Dwarf_Handler  dwarf_handler;
    typedef ::Dwarf_Error    dwarf_error;
    typedef ::Dwarf_Die      dwarf_die;
    typedef ::Dwarf_Line     dwarf_line;
    typedef ::Dwarf_Ptr      dwarf_pointer;
    typedef ::Dwarf_Addr     dwarf_address;
    typedef ::Dwarf_Off      dwarf_offset;
    typedef ::Dwarf_Half     dwarf_half;
    typedef ::Dwarf_Signed   dwarf_signed;
    typedef ::Dwarf_Unsigned dwarf_unsigned;
    typedef ::Dwarf_Bool     dwarf_bool;
    typedef ::Dwarf_Sig8     dwarf_sig8;
    typedef ::Dwarf_Line     dwarf_line;
    typedef ::Dwarf_Half     dwarf_tag;
    typedef ::Dwarf_Half     dwarf_attr;
  }
}

#endif
