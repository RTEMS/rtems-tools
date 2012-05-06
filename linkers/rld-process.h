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

namespace rld
{
  namespace process
  {
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
      typedef std::list < std::string > tempfile_container;
      
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
      const std::string get ();

      /**
       * Remove the temporary file.
       */
      void erase (const std::string& name);

      /**
       * Remove all temporary files.
       */
      void clean_up ();

    private:

      /*
       * Delete the file.
       */
      void unlink (const std::string& name);

      tempfile_container tempfiles; //< The temporary files.

    };

    /**
     * The temporary files.
     */
    static temporary_files temporaries;

    /**
     * Handle the output files from the process.
     */
    class tempfile
    {
    public:

      /**
       * Get a temporary file name.
       */
      tempfile ();

      /**
       * Clean up the temporary file.
       */
      ~tempfile ();

      /**
       * Open the temporary file.
       */
      void open ();

      /**
       * Close the temporary file.
       */
      void close ();

      /**
       * The name of the temp file.
       */
      const std::string& name () const;

      /**
       * Size of the file.
       */
      size_t size ();

      /**
       * Get all the file.
       */
      void get (std::string& all);

      /**
       * Get time.
       */
      void getline (std::string& line);

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

      std::string _name;    //< The name of the file.
      int         fd;       //< The file descriptor
      char        buf[256]; //< The read buffer.
      int         level;    //< The level of data in the buffer.
    };
    
    /**
     * The arguments containter has a single argument per element.
     */
    typedef std::vector < std::string > arg_container;

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
