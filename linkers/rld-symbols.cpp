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
 * @brief RTEMS Linker symbols manages the symbols from all the object files.
 *
 */

#include <string.h>

#include <iomanip>

#include <rld.h>

#include <libiberty/demangle.h>

namespace rld
{
  namespace symbols
  {
    /**
     * Get the demangled name.
     */
    static void
    denamgle_name (std::string& name, std::string& demangled)
    {
      char* demangled_name = ::cplus_demangle (name.c_str (),
                                               DMGL_ANSI | DMGL_PARAMS);
      if (demangled_name)
      {
        demangled = demangled_name;
        ::free (demangled_name);
      }
    }

    symbol::symbol ()
      : object_ (0),
        references_ (0)
    {
      memset (&esym_, 0, sizeof (esym_));
    }

    symbol::symbol (const std::string&  name,
                    files::object&      object,
                    const elf::elf_sym& esym)
      : name_ (name),
        object_ (&object),
        esym_ (esym),
        references_ (0)
    {
      if (!object_)
        throw rld_error_at ("object pointer is 0");
      if (name_.empty ())
        throw rld_error_at ("name is empty in " + object.name ().full ());
      if (is_cplusplus ())
        denamgle_name (name_, demangled_);
    }

    symbol::symbol (const std::string& name, const elf::elf_sym& esym)
      : name_ (name),
        object_ (0),
        esym_ (esym),
        references_ (0)
    {
      if (name_.empty ())
        throw rld_error_at ("name is empty");
      if (is_cplusplus ())
        denamgle_name (name_, demangled_);
    }

    symbol::symbol (const std::string&  name,
                    const elf::elf_addr value)
      : name_ (name),
        object_ (0),
        references_ (0)
    {
      memset (&esym_, 0, sizeof (esym_));
      esym_.st_value = value;
    }

    symbol::symbol (const char*         name,
                    const elf::elf_addr value)
      : name_ (name),
        object_ (0),
        references_ (0)
    {
      memset (&esym_, 0, sizeof (esym_));
      esym_.st_value = value;
    }

    const std::string&
    symbol::name () const
    {
      return name_;
    }

    const std::string&
    symbol::demangled () const
    {
      return demangled_;
    }

    bool
    symbol::is_cplusplus () const
    {
      return (name_[0] == '_') && (name_[1] == 'Z');
    }

    int
    symbol::type () const
    {
      return GELF_ST_TYPE (esym_.st_info);
    }

    int
    symbol::binding () const
    {
      return GELF_ST_BIND (esym_.st_info);
    }

    int
    symbol::index () const
    {
      return esym_.st_shndx;
    }

    rld::files::object*
    symbol::object () const
    {
      return object_;
    }

    void
    symbol::set_object (rld::files::object& obj)
    {
      object_ = &obj;
    }

    const elf::elf_sym&
    symbol::esym () const
    {
      return esym_;
    }

    void
    symbol::referenced ()
    {
      ++references_;
      if (object_)
        object_->symbol_referenced ();
    }

    bool
    symbol::operator< (const symbol& rhs) const
    {
      return name_ < rhs.name_;
    }

    void
    symbol::output (std::ostream& out) const
    {
      const elf::elf_sym& es = esym ();

      std::string binding;
      int binding_val = GELF_ST_BIND (es.st_info);
      switch (binding_val)
      {
        case STB_LOCAL:
          binding = "STB_LOCAL ";
          break;
        case STB_GLOBAL:
          binding = "STB_GLOBAL";
          break;
        case STB_WEAK:
          binding = "STB_WEAK  ";
          break;
        default:
          if ((binding_val >= STB_LOPROC) && (binding_val <= STB_HIPROC))
            binding = "STB_LOPROC(" + rld::to_string (binding_val) + ")";
          else
            binding = "STB_INVALID("  + rld::to_string (binding_val) + ")";
          break;
      }

      std::string type;
      int type_val = GELF_ST_TYPE (es.st_info);
      switch (type_val)
      {
        case STT_NOTYPE:
          type = "STT_NOTYPE ";
          break;
        case STT_OBJECT:
          type = "STT_OBJECT ";
          break;
        case STT_FUNC:
          type = "STT_FUNC   ";
          break;
        case STT_SECTION:
          type = "STT_SECTION";
          break;
        case STT_FILE:
          type = "STT_FILE   ";
          break;
        default:
          if ((type_val >= STT_LOPROC) && (type_val <= STT_HIPROC))
            type = "STT_LOPROC(" + rld::to_string (type_val) + ")";
          else
            type = "STT_INVALID("  + rld::to_string (type_val) + ")";
          break;
      }

      out << binding
          << ' ' << type
          << " 0x" << std::setw (8) << std::setfill ('0') << std::hex
          << es.st_value
          << std::dec << std::setfill (' ')
          << ' ' << std::setw (7)  << es.st_size
          << ' ';

      if (is_cplusplus ())
        out << demangled ();
      else
        out << name ();

      if (object ())
        out << "   (" << object ()->name ().basename () << ')';
    }

    void
    load (bucket& bucket_, table& table_)
    {
      for (bucket::iterator sbi = bucket_.begin ();
           sbi != bucket_.end ();
           ++sbi)
      {
        symbol& sym = *sbi;
        table_[sym.name ()] = &sym;
      }
    }

    size_t
    referenced (pointers& symbols)
    {
      size_t used = 0;
      for (pointers::iterator sli = symbols.begin ();
           sli != symbols.end ();
           ++sli)
      {
        symbol& sym = *(*sli);
        if (sym.references ())
          ++used;
      }

      return used;
    }

    void
    output (std::ostream& out, const table& symbols)
    {
      std::cout << " No.  Scope      Type        Address    Size    Name" << std::endl;
      int index = 0;
      for (table::const_iterator si = symbols.begin ();
           si != symbols.end ();
           ++si)
      {
        const symbol& sym = *((*si).second);
        out << std::setw (5) << index << ' ' << sym << std::endl;
        ++index;
      }
    }

  }
}
