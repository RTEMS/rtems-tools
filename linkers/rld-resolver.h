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
 * @brief RTEMS Linker resolver determines which object files are needed.
 *
 */

#if !defined (_RLD_RESOLVER_H_)
#define _RLD_RESOLVER_H_

#include <rld-files.h>
#include <rld-symbols.h>

namespace rld
{
  namespace resolver
  {
    /**
     * Resolve the dependences between object files.
     *
     * @param dependents The object modules dependent on the object files we
     *                   are linking.
     * @param cache The file cache.
     * @param base_symbols The base image symbol table
     * @param symbols The object file and library symbols
     * @param undefined Extra undefined symbols dependent object files are
     *                  added for.
     */
    void resolve (rld::files::object_list& dependents, 
                  rld::files::cache&       cache,
                  rld::symbols::table&     base_symbols,
                  rld::symbols::table&     symbols,
                  rld::symbols::table&     undefined);
  }
}

#endif
