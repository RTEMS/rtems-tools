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
      : index_ (-1),
        object_ (0),
        references_ (0)
    {
      memset (&esym_, 0, sizeof (esym_));
    }

    symbol::symbol (int                 index,
                    const std::string&  name,
                    files::object&      object,
                    const elf::elf_sym& esym)
      : index_ (index),
        name_ (name),
        object_ (&object),
        esym_ (esym),
        references_ (0)
    {
      if (!object_)
        throw rld_error_at ("object pointer is 0");
      if (is_cplusplus ())
        denamgle_name (name_, demangled_);
    }

    symbol::symbol (int                 index,
                    const std::string&  name,
                    const elf::elf_sym& esym)
      : index_ (index),
        name_ (name),
        object_ (0),
        esym_ (esym),
        references_ (0)
    {
      if (is_cplusplus ())
        denamgle_name (name_, demangled_);
    }

    symbol::symbol (const std::string&  name,
                    const elf::elf_addr value)
      : index_ (-1),
        name_ (name),
        object_ (0),
        references_ (0)
    {
      memset (&esym_, 0, sizeof (esym_));
      esym_.st_value = value;
    }

    symbol::symbol (const char*         name,
                    const elf::elf_addr value)
      : index_ (-1),
        name_ (name),
        object_ (0),
        references_ (0)
    {
      memset (&esym_, 0, sizeof (esym_));
      esym_.st_value = value;
    }

    int
    symbol::index () const
    {
      return index_;
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

    bool
    symbol::is_local () const
    {
      return binding () == STB_LOCAL;
    }

    bool
    symbol::is_weak () const
    {
      return binding () == STB_WEAK;
    }

    bool
    symbol::is_global () const
    {
      return binding () == STB_GLOBAL;
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
    symbol::section_index () const
    {
      return esym_.st_shndx;
    }

    elf::elf_addr
    symbol::value () const
    {
      return esym_.st_value;
    }

    uint32_t
    symbol::info () const
    {
      return esym_.st_info;
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

      out << std::setw (5) << index_
          << ' ' << binding
          << ' ' << type
          << ' ' << std::setw(6) << es.st_shndx
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

    table::table ()
    {
    }

    table::~table ()
    {
    }

    void
    table::add_global (symbol& sym)
    {
      globals_[sym.name ()] = &sym;
    }

    void
    table::add_weak (symbol& sym)
    {
      weaks_[sym.name ()] = &sym;
    }

    void
    table::add_local (symbol& sym)
    {
      locals_[sym.name ()] = &sym;
    }

    symbol*
    table::find_global (const std::string& name)
    {
      symtab::iterator sti = globals_.find (name);
      if (sti == globals_.end ())
        return 0;
      return (*sti).second;
    }

    symbol*
    table::find_weak (const std::string& name)
    {
      symtab::iterator sti = weaks_.find (name);
      if (sti == weaks_.end ())
        return 0;
      return (*sti).second;
    }

    symbol*
    table::find_local (const std::string& name)
    {
      symtab::iterator sti = locals_.find (name);
      if (sti == locals_.end ())
        return 0;
      return (*sti).second;
    }

    size_t
    table::size () const
    {
      return globals_.size () + weaks_.size () + locals_.size ();
    }

    const symtab&
    table::globals () const
    {
      return globals_;
    }

    const symtab&
    table::weaks () const
    {
      return weaks_;
    }

    const symtab&
    table::locals () const
    {
      return locals_;
    }

    void
    table::globals (addrtab& addresses)
    {
      for (symtab::iterator gi = globals_.begin ();
           gi != globals_.end ();
           ++gi)
      {
        symbol& sym = *((*gi).second);
        addresses[sym.value ()] = (*gi).second;
      }
    }

    void
    table::weaks (addrtab& addresses)
    {
      for (symtab::iterator wi = weaks_.begin ();
           wi != weaks_.end ();
           ++wi)
      {
        symbol& sym = *((*wi).second);
        addresses[sym.value ()] = (*wi).second;
      }
    }

    void
    table::locals (addrtab& addresses)
    {
      for (symtab::iterator li = locals_.begin ();
           li != locals_.end ();
           ++li)
      {
        symbol& sym = *((*li).second);
        addresses[sym.value ()] = (*li).second;
      }
    }

    void
    load (bucket& bucket_, table& table_)
    {
      for (bucket::iterator sbi = bucket_.begin ();
           sbi != bucket_.end ();
           ++sbi)
      {
        table_.add_global (*sbi);
      }
    }

    void
    load (bucket& bucket_, symtab& table_)
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
      out << "Globals:" << std::endl;
      output (out, symbols.globals ());
      out << "Weaks:" << std::endl;
      output (out, symbols.weaks ());
      out << "Locals:" << std::endl;
      output (out, symbols.locals ());
    }

    void
    output (std::ostream& out, const symtab& symbols)
    {
      out << " No.  Index Scope      Type        SHNDX  Address    Size    Name" << std::endl;
      int index = 0;
      for (symtab::const_iterator si = symbols.begin ();
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
