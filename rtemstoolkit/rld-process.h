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
 * @brief Process execute and various supporting parts.
 *
 */

#if !defined (_RLD_PEX_H_)
#define _RLD_PEX_H_

#include <list>
#include <string>
#include <vector>
#include "rld.h"

namespace rld
{
  namespace process
  {
    /**
     * Temporary file is a name and keep state.
     */
    struct tempfile_ref
    {
      const std::string name; //< The name of the tempfile.
      bool              keep; //< If true do not delete this file.

      tempfile_ref (const std::string& name, bool keep = false)
        : name (name),
          keep (keep) {
      }
    };

    /**
     * Manage temporary files. We keep these so we can delete them when
     * we exit.
     */
    class temporary_files
    {
    public:
      /**
       * Container of temporary file names.
       */
      typedef std::list < tempfile_ref > tempfile_container;

      /**
       * Construct the temporary files.
       */
      temporary_files ();

      /**
       * Destruct cleaning up.
       */
      ~temporary_files ();

      /**
       * Get a new temporary file name.
       */
      const std::string get (const std::string& suffix = ".rldxx",
                             bool               keep = false);

      /**
       * Remove the temporary file.
       */
      void erase (const std::string& name);

      /**
       * Set the tempfile reference keep state to true.
       */
      void keep (const std::string& name);

      /**
       * Remove all temporary files.
       */
      void clean_up ();

    private:

      /*
       * Delete the tempfile given the reference if not keeping.
       */
      void unlink (const tempfile_ref& ref);

      tempfile_container tempfiles; //< The temporary files.

    };

    /**
     * Handle the output files from the process.
     */
    class tempfile
    {
    public:

      /**
       * Get a temporary file name given a suffix.
       */
      tempfile (const std::string& suffix = ".rldxx", bool keep = false);

      /**
       * Clean up the temporary file.
       */
      ~tempfile ();

      /**
       * Open the temporary file. It can optionally be written too.
       */
      void open (bool writable = false);

      /**
       * Close the temporary file.
       */
      void close ();

      /**
       * Override the temp file name automatically assigned with this name. The
       * suffix is appended.
       */
      void override (const std::string& name);

      /**
       * Set the temp file keep state to true so it is not deleted.
       */
      void keep ();

      /**
       * The name of the temp file.
       */
      const std::string& name () const;

      /**
       * Size of the file.
       */
      size_t size ();

      /**
       * Read all the file.
       */
      void read (std::string& all);

      /**
       * Read a line at a time.
       */
      void read_line (std::string& line);

      /**
       * Write the string to the file.
       */
      void write (const std::string& s);

      /**
       * Write the string as a line to the file.
       */
      void write_line (const std::string& s);

      /**
       * Write the strings to the file using a suitable line separator.
       */
      void write_lines (const rld::strings& ss);

      /**
       * Output the file.
       */
      void output (const std::string& prefix,
                   std::ostream&      out,
                   bool               line_numbers = false);

      /**
       * Output the file.
       */
      void output (std::ostream& out);

    private:

      std::string       _name;      //< The name of the file.
      const std::string suffix;     //< The temp file's suffix.
      bool              overridden; //< The name is overridden; may no exist.
      int               fd;         //< The file descriptor
      char              buf[256];   //< The read buffer.
      size_t            level;      //< The level of data in the buffer.
    };

    /**
     * Keep the temporary files.
     */
    void set_keep_temporary_files ();

    /**
     * Clean up the temporaryes.
     */
    void temporaries_clean_up ();

    /**
     * The arguments containter has a single argument per element.
     */
    typedef std::vector < std::string > arg_container;

    /**
     * Split a string and append to the arguments.
     */
    void args_append (arg_container& args, const std::string& str);

    /**
     * Execute result.
     */
    struct status
    {
      enum types
      {
        normal, //< The process terminated normally by a call to _exit(2) or exit(3).
        signal, //< The process terminated due to receipt of a signal.
        stopped //< The process has not terminated, but has stopped and can be restarted.
      };

      types type; //< Type of status.
      int   code; //< The status code returned.
    };

    /**
     * Execute a process and capture stdout and stderr. The first element is
     * the program name to run. Return an error code.
     */
    status execute (const std::string&   pname,
                    const arg_container& args,
                    const std::string&   outname,
                    const std::string&   errname);

    /**
     * Execute a process and capture stdout and stderr given a command line
     * string. Return an error code.
     */
    status execute (const std::string& pname,
                    const std::string& command,
                    const std::string& outname,
                    const std::string& errname);

    /**
     * Parse a command line into arguments. It support quoting.
     */
    void parse_command_line (const std::string& command, arg_container& args);
  }
}

#endif
