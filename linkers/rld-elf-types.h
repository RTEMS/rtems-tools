/*
 * Copyright (c) 2011, Chris Johns <chrisj@rtems.org>
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
 * @brief RTEMS Linker ELF types.
  *
 */

#if !defined (_RLD_ELF_TYPES_H_)
#define _RLD_ELF_TYPES_H_

#define __LIBELF_INTERNAL__ 1
#include <gelf.h>
#include <libelf.h>

namespace rld
{
  namespace elf
  {
    /**
     * Hide the types from libelf we use.
     */
    typedef ::GElf_Half   elf_half;
    typedef ::GElf_Word   elf_word;
    typedef ::GElf_Xword  elf_xword;
    typedef ::GElf_Sxword elf_sxword;
    typedef ::Elf_Type    elf_type;
    typedef ::GElf_Addr   elf_addr;
    typedef ::GElf_Off    elf_off;
    typedef ::GElf_Sym    elf_sym;
    typedef ::Elf_Kind    elf_kind;
    typedef ::Elf_Scn     elf_scn;
    typedef ::GElf_Ehdr   elf_ehdr;
    typedef ::Elf32_Ehdr  elf32_ehdr;
    typedef ::GElf_Shdr   elf_shdr;
    typedef ::GElf_Phdr   elf_phdr;
    typedef ::Elf_Data    elf_data;
    typedef ::GElf_Rel    elf_rel;
    typedef ::GElf_Rela   elf_rela;
    typedef ::Elf         elf;
  }
}

#endif
