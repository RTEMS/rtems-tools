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
/**
 * @file
 *
 * @ingroup rtems-ld
 *
 * @brief RTEMS Linker Path to help manage paths.
 *
 */

#if !defined (_RLD_PATH_H_)
#define _RLD_PATH_H_

#include <list>
#include <map>
#include <string>
#include <vector>

#include <rld.h>

namespace rld
{
  namespace path
  {
    /**
     * Container of file paths.
     */
    typedef std::vector < std::string > paths;

    /**
     * Return the basename of the file name.
     *
     * @param name The full file name.
     * @return const std::string The basename of the file.
     */
    const std::string basename (const std::string& name);

    /**
     * Return the dirname of the file name.
     *
     * @param name The full file name.
     * @return const std::string The dirname of the file.
     */
    const std::string dirname (const std::string& name);

    /**
     * Return the extension of the file name.
     *
     * @param name The full file name.
     * @return const std::string The extension of the file.
     */
    const std::string extension (const std::string& name);

    /**
     * Split a path from a string with the path seperator to the path
     * container. Add only the paths that exist and ignore those that do not.
     *
     * @param path The paths as a single string delimited by the path
     *             separator.
     * @param paths The split path paths.
     */
    void path_split (const std::string& path,
                     paths&             paths);

    /**
     * Make a path by joining the base and part with required separator.
     *
     * @param base The path component to be joined.
     * @param part The file name to add to the path.
     * @param joined The joined path and file name with a path separator.
     */
    void path_join (const std::string& base,
                    const std::string& part,
                    std::string&       joined);

    /**
     * Make a path by joining the parts with the base and the required
     * separator.
     *
     * @param base The path component to be joined.
     * @param parts The files to add to the path.
     * @param joined The joined path and file name with a path separator.
     */
    void path_join (const std::string& base,
                    const paths&       parts,
                    std::string&       joined);

    /**
     * Return the absolute path given a path and using the current working
     * directory. The path is flattened removing any '..' sequences.
     *
     * @param path The path to be return as absolute.
     * @return const std::string The absolute path.
     */
    const std::string path_abs (const std::string& path);

    /**
     * Check the path is a file using a stat call.
     *
     * @param path The path to check.
     * @retval true The path is valid.
     * @retval false The path is not valid.
     */
    bool check_file (const std::string& path);

    /**
     * Check if the path is a directory.
     *
     * @param path The path to check.
     * @retval false The path is not a directory.
     * @retval true The path is a directory.
     */
    bool check_directory (const std::string& path);

    /**
     * Find the file given a container of paths and file names.
     *
     * @param path The path of the file if found else empty.
     * @param name The name of the file to search for.
     * @param search_paths The container of paths to search.
     */
    void find_file (std::string&       path,
                    const std::string& name,
                    paths&             search_paths);

    /**
     * Unlink the file.
     *
     * @param path The path of the file to unlink.
     */
    void unlink (const std::string& path, bool not_present_error = false);

  }
}

#endif
