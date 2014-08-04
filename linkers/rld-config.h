/*
 * Copyright (c) 2014, Chris Johns <chrisj@rtems.org>
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
 * @ingroup rtems_rld
 *
 * @brief INI Configuration reader.
 *
 */

#if !defined (_RLD_CONFIG_H_)
#define _RLD_CONFIG_H_

#include <string>
#include <list>
#include <vector>

#include <rld.h>

namespace rld
{
  namespace config
  {
    /**
     * The configuration item. This is a data component of a record contained
     * in a section.
     */
    struct item
    {
      std::string text; /**< The text as read from the configuration. */

      /**
       * Construct an item.
       */
      item (const std::string& text);
      item (const char*        text);
    };

    /**
     * Configuration item container.
     */
    typedef std::vector < item > items;

    /**
     * Configuration record is a line in a section. There can be multiple
     * records with the same key in a section. Keys are specific to a section.
     */
    struct record
    {
      std::string name;   //< Name of the record.
      items       items;  //< The record's items.

      /**
       * Return true if there is only one item.
       */
      bool single () const {
        return items.size () == 1;
      }
    };

    /**
     * Configuration record container.
     */
    typedef std::list < record > records;

    /**
     * Configuration section. A section contains a number of records and the
     * records contain [1..n] items.
     */
    struct section
    {
      std::string name;  //< Name of the section.
      records     recs;  //< The section's records.

      /**
       * Find a record and throw an error if not found.
       */
      const record& get_record (const std::string& name) const;

      /**
       * Return the single item in a record. If the record is duplicated an
       * error is thrown.
       */
      std::string get_record_item (const std::string& name) const;

      /**
       * Return the list of items in a record in a strings container.
       */
      void get_record_items (const std::string& name, rld::strings& items) const;
    };

    /**
     * Configuration section container.
     */
    typedef std::list < section > sections;

    /**
     * Container of configuration file paths loaded.
     */
    typedef std::vector < std::string > paths;

    /**
     * The configuration.
     */
    class config
    {
    public:
      /**
       * Construct an empty configuration.
       */
      config();
      virtual ~config();

      /**
       * Clear the current configuration.
       */
      void clear ();

      /**
       * Load a configuration.
       */
      void load (const std::string& name);

      /**
       * Process any include records in the section named. If the section has
       * any records named 'include' split the items and include the
       * configuration files.
       */
      void includes (const section& sec, bool must_exist = false);

      /**
       * Get the section and throw an error if not found.
       */
      const section& get_section (const std::string& name) const;

      /**
       * Get the paths of loaded configuration files.
       */
      const paths& get_paths () const;

    private:

      paths    paths; /**< The path's of the loaded files. */
      sections secs;  /**< The sections loaded from configuration files */
    };

    /**
     * Return the items from a record.
     */
    template < typename T >
    void parse_items (const rld::config::record& record, T& items)
    {
      items.clear ();
      for (rld::config::items::const_iterator ii = record.items.begin ();
           ii != record.items.end ();
           ++ii)
      {
        rld::strings ss;
        rld::split (ss, (*ii).text, ',');
        std::copy (ss.begin (), ss.end (), std::back_inserter (items));
      }
    }

    /**
     * Return the items from a record in a section. Optionally raise an error
     * if the record is not found and it is to be present.
     */
    template < typename T >
    void parse_items (const rld::config::section& section,
                      const std::string&          name,
                      T&                          items,
                      bool                        present = false)
    {
      items.clear ();
      const rld::config::record* rec = 0;
      try
      {
        const rld::config::record& rr = section.get_record (name);
        rec = &rr;
      }
      catch (rld::error re)
      {
        /*
         * Ignore the error if it does not need to exist.
         */
        if (present)
          throw rld::error ("not found", "record: " + section.name + name);
      }

      if (rec)
        parse_items (*rec, items);
    }

    /**
     * Return the items from a record in a section in the
     * configuration. Optionally raise an error if the section is not found and
     * it is to be present.
     */
    template < typename T >
    void parse_items (const rld::config::config& config,
                      const std::string&         section,
                      const std::string&         record,
                      T&                         items,
                      bool                       present = false)
    {
      items.clear ();
      const rld::config::section* sec = 0;
      try
      {
        const rld::config::section& sr = config.get_section (section);
        sec = &sr;
      }
      catch (rld::error re)
      {
        /*
         * Ignore the error if it does not need to exist.
         */
        if (present)
          throw rld::error ("not found", "section: " + section);
      }

      if (sec)
        parse_items (*sec, record, items);
    }
  }
}

#endif
