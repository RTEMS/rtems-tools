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
 * @brief RTEMS Linker file manages access the image contained in various file
 * formats.
 *
 * The base element is a file. It references a object file that is either
 * inside an archive or stand alone. You access a object file by constructing a
 * handle. A handle is the object file with the specific file descriptor
 * created when the archive or object file was opened.
 *
 *
 */

#if !defined (_RLD_FILES_H_)
#define _RLD_FILES_H_

#include <list>
#include <map>
#include <string>
#include <vector>

#include <rld.h>

namespace rld
{
  namespace files
  {
    /**
     * Container of file paths.
     */
    typedef std::vector < std::string > paths;

    /**
     * Container of files.
     */
    typedef std::vector < file > files;

    /**
     * Container of archive files.
     */
    typedef std::map < std::string, archive* > archives;

    /**
     * Container of object files.
     */
    typedef std::map < std::string, object* > objects;

    /**
     * Container list of object files.
     */
    typedef std::list < object* > object_list;

    /**
     * Return the basename of the file name.
     *
     * @param name The full file name.
     * @return std::string The basename of the file.
     */
    std::string basename (const std::string& name);

    /**
     * Return the dirname of the file name.
     *
     * @param name The full file name.
     * @return std::string The dirname of the file.
     */
    std::string dirname (const std::string& name);

    /**
     * Return the extension of the file name.
     *
     * @param name The full file name.
     * @return std::string The extension of the file.
     */
    std::string extension (const std::string& name);

    /**
     * Split a path from a string with a delimiter to the path container. Add
     * only the paths that exist and ignore those that do not.
     *
     * @param path The paths as a single string delimited by the path
     *             separator.
     * @param paths The split path paths.
     */
    void path_split (const std::string& path,
                     rld::files::paths& paths);

    /**
     * Make a path by joining the parts with required separator.
     */
    void path_join (const std::string& path_,
                    const std::string& file_,
                    std::string& joined);

    /**
     * Check the path is a file.
     */
    bool check_file (const std::string& path);

    /**
     * Check the path is a directory.
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
     * A file is a single object file that is either in an archive or stand
     * alone.
     */
    class file
    {
    public:
      /**
       * Construct the file from the component parts when part of an archive.
       *
       * @param aname The archive name.
       * @param oname The object file name.
       * @param offset The offset in the archive the object file starts.
       * @param size The size of the archive the object file starts.
       */
      file (const std::string& aname,
            const std::string& oname,
            off_t              offset,
            size_t             size);

      /**
       * Construct the name by splitting the full path into an archive,
       * object file name and offset.
       *
       * @param path The path to the image.
       * @param is_object If true (default) the name is for an object file.
       */
      file (const std::string& path, bool is_object = true);

      /**
       * Contruct an empty file.
       */
      file ();

      /**
       * Set a name from the path.
       *
       * @param path The path to the image.
       * @param is_object If true (default) the name is for an object file.
       */
      void set (const std::string& path, bool is_object = true);

      /**
       * Is an archive returns true if the file is in an archive.
       *
       * @retval true The object file is in an archive.
       * @retval false The object file is stand alone.
       */
      bool is_archive () const;

      /**
       * Is object file stand alone.
       *
       * @retval true The object file is stand alone.
       * @retval false The object could be part of an archive.
       */
      bool is_object () const;

      /**
       * Valid returns true if there is a valid name.
       *
       * @retval true There is a valid name.
       * @retval false There is not name assigned.
       */
      bool is_valid () const;

      /**
       * Exists returns true if the archive or object file is present on disk
       * and a regular file.
       *
       * @retval true The file is valid and a regular file.
       * @retval false The file is either not present, not accessable or not a
       *               regular file.
       */
      bool exists () const;

      /**
       * The path maps to the real file on disk. The file may not be valid.
       *
       * @return const std::string The real path to the file on disk.
       */
      const std::string path () const;

      /**
       * The full path.
       *
       * @return const std::string The full path to the image.
       */
      const std::string full () const;

      /**
       * The base path. It is the basename of the full path.
       *
       * @return const std::string The basename of the full path to the image.
       */
      const std::string basename () const;

      /**
       * The archive name component. A length of 0 means there was not
       * archive component.
       */
      const std::string& aname () const;

      /**
       * The object name. There is always an object name.
       */
      const std::string& oname () const;

      /**
       * The object's offset in the archive.
       */
      off_t offset () const;

      /**
       * The object's size in the archive.
       */
      size_t size () const;

    private:
      std::string aname_;  //< The archive name.
      std::string oname_;  //< The object name.
      off_t       offset_; //< The object's offset in the archive.
      size_t      size_;   //< The object's size in teh archive.
    };

    /**
     * Image is the base file type. A base class is used so we can have a
     * single container of to hold types of images we need to support.
     */
    class image
    {
    public:
      /**
       * Construct the image.
       *
       * @param name The name of the image.
       */
      image (file& name);

      /**
       * Construct the image.
       *
       * @param name The file path.
       * @param is_object If true (default) the name is for an object file.
       */
      image (const std::string& path, bool is_object = true);

      /**
       * Construct the image.
       */
      image ();

      /**
       * Destruct the image.
       */
      virtual ~image ();

      /**
       * Open the image. You can open the image more than once but you need to
       * close it the same number of times.
       */
      virtual void open (file& name);

      /**
       * Open the image. You can open the image more than once but you need to
       * close it the same number of times.
       */
      virtual void open (bool writable = false);

      /**
       * Close the image.
       */
      virtual void close ();

      /**
       * Read a block from the file.
       */
      virtual ssize_t read (uint8_t* buffer, size_t size);

      /**
       * Write a block from the file.
       */
      virtual ssize_t write (const void* buffer, size_t size);

      /**
       * Seek.
       */
      virtual void seek (off_t offset);

      /**
       * Seek and read.
       */
      virtual bool seek_read (off_t offset, uint8_t* buffer, size_t size);

      /**
       * Seek and write.
       */
      virtual bool seek_write (off_t offset, const void* buffer, size_t size);

      /**
       * The name of the image.
       */
      const file& name () const;

      /**
       * References to the image.
       */
      virtual int references () const;

      /**
       * The file size.
       */
      virtual size_t size () const;

      /**
       * The file descriptor.
       */
      virtual int fd () const;

      /**
       * The ELF reference.
       */
      elf::file& elf ();

      /**
       * A symbol in the image has been referenced.
       */
      virtual void symbol_referenced ();

      /**
       * Return the number of symbol references.
       */
      virtual int symbol_references () const;

      /**
       * The path maps to the real file on disk. The file may not be valid.
       *
       * @return const std::string The real path to the file on disk.
       */
      const std::string path () const {
        return name ().path ();
      }

      /**
       * Is the image open ?
       *
       * @retval true The image is open.
       * @retval false The image is not open.
       */
      bool is_open () const {
        return fd () != -1;
      }

      /**
       * Is the image writable ?
       *
       * @retval true The image is writable.
       * @retval false The image is not writable.
       */
      bool is_writable () const {
        return writable;
      }

    private:

      file      name_;       //< The name of the file.
      int       references_; //< The number of handles open.
      int       fd_;         //< The file descriptor of the archive.
      elf::file elf_;        //< The libelf reference.
      int       symbol_refs; //< The number of symbols references made.
      bool      writable;    //< The image is writable.
    };

    /**
     * Copy the input section of the image to the output section. The file
     * positions in the images must be set before making the call.
     */
    void copy (image& in, image& out, size_t size);

    /**
     * The archive class proivdes access to ELF object files that are held in a
     * AR format file. GNU AR extensions are supported.
     */
    class archive:
      public image
    {
    public:
      /**
       * Open a archive format file that contains ELF object files.
       *
       */
      archive (const std::string& name);

      /**
       * Close the archive.
       */
      virtual ~archive ();

      /**
       * Begin the ELF session.
       */
      void begin ();

      /**
       * End the ELF session.
       */
      void end ();

      /**
       * Match the archive name.
       *
       * @param name The name of the archive to check.
       * @retval true This is the archive.
       * @retval false This is not the archive.
       */
      bool is (const std::string& name) const;

      /**
       * Check this is a valid archive.
       */
      bool is_valid ();

      /**
       * Load objects.
       */
      void load_objects (objects& objs);

      /**
       * Get the name.
       */
      const std::string& get_name () const;

      /**
       * Less than operator for the map container.
       */
      bool operator< (const archive& rhs) const;

      /**
       * Create a new archive. If referening an existing archive it is
       * overwritten.
       */
      void create (object_list& objects);

    private:

      /**
       * Read header.
       */
      bool read_header (off_t offset, uint8_t* header);

      /**
       * Add the object file from the archive to the object's container.
       */
      void add_object (objects&    objs,
                       const char* name,
                       off_t       offset,
                       size_t      size);

      /**
       * Write a file header into the archive.
       */
      void write_header (const std::string& name,
                         uint32_t           mtime,
                         int                uid,
                         int                gid,
                         int                mode,
                         size_t             size);

      /**
       * Cannot copy via a copy constructor.
       */
      archive (const archive& orig);

      /**
       * Cannot assign using the assignment operator..
       */
      archive& operator= (const archive& rhs);
    };

    /**
     * The object file cab be in an archive or a file.
     */
    class object:
      public image
    {
    public:
      /**
       * Construct an object image that is part of an archive.
       *
       * @param archive_ The archive the object file is part of.
       * @param file_ The image file.
       */
      object (archive& archive_, file& file_);

      /**
       * Construct the object file.
       *
       * @param path The object file path.
       */
      object (const std::string& path);

      /**
       * Construct the object file.
       */
      object ();

      /**
       * Destruct the object file.
       */
      virtual ~object ();

      /**
       * Open the object file.
       */
      virtual void open (bool writable = false);

      /**
       * Close the object.
       */
      virtual void close ();

      /**
       * Begin the ELF session.
       */
      void begin ();

      /**
       * End the ELF session.
       */
      void end ();

      /**
       * Load the symbols into the symbols table.
       *
       * @param symbols The symbol table to load.
       * @param local Include local symbols. The default is not to.
       */
      void load_symbols (rld::symbols::table& symbols, bool local = false);

      /**
       * References to the image.
       */
      virtual int references () const;

      /**
       * The file size.
       */
      virtual size_t size () const;

      /**
       * The file descriptor.
       */
      virtual int fd () const;

      /**
       * A symbol in the image has been referenced.
       */
      virtual void symbol_referenced ();

      /**
       * The archive the object file is contained in. If 0 the object file is
       * not contained in an archive.
       */
      archive* get_archive ();

      /**
       * Return the unresolved symbol table for this object file.
       */
      rld::symbols::table& unresolved_symbols ();

      /**
       * Return the list external symbols.
       */
      rld::symbols::pointers& external_symbols ();

    private:
      archive*               archive_;   //< Points to the archive if part of
                                         //  an archive.
      rld::symbols::table    unresolved; //< This object's unresolved symbols.
      rld::symbols::pointers externals;  //< This object's external symbols.

      /**
       * Cannot copy via a copy constructor.
       */
      object (const object& orig);

      /**
       * Cannot assign using the assignment operator.
       */
      object& operator= (const object& rhs);
    };

    /**
     * A collection of objects files as a cache. This currently is not a cache
     * but it could become one.
     */
    class cache
    {
    public:
      /**
       * Construct the cache.
       */
      cache ();

      /**
       * Destruct the objects.
       */
      virtual ~cache ();

      /**
       * Open the cache by collecting the file names, loading object headers
       * and loading the archive file names.
       */
      void open ();

      /**
       * Close the cache.
       */
      void close ();

      /**
       * Add a file path to the cache.
       */
      void add (const std::string& path);

      /**
       * Add a container of path to the cache.
       */
      void add (paths& paths__);

      /**
       * Add a container of path to the cache.
       */
      void add_libraries (paths& paths__);

      /**
       * Being a session on an archive.
       */
      void archive_begin (const std::string& path);

      /**
       * End a session on an archive.
       */
      void archive_end (const std::string& path);

      /**
       * Being sessions on all archives.
       */
      void archives_begin ();

      /**
       * End the archive sessions.
       */
      void archives_end ();

      /**
       * Collect the object names and add them to the cache.
       */
      void collect_object_files ();

      /**
       * Collect the object file names by verifing the paths to the files are
       * valid or read the object file names contained in any archives.
       */
      void collect_object_files (const std::string& path);

      /**
       * Load the symbols into the symbol table.
       *
       * @param symbols The symbol table to load.
       * @param local Include local symbols. The default is not to.
       */
      void load_symbols (rld::symbols::table& symbols, bool locals = false);

      /**
       * Output the unresolved symbol table to the output stream.
       */
      void output_unresolved_symbols (std::ostream& out);

      /**
       * Get the archives.
       */
      archives& get_archives ();

      /**
       * Get the objects inlcuding those in archives.
       */
      objects& get_objects ();

      /**
       * Get the added objects. Does not include the ones in th archives.
       */
      void get_objects (object_list& list);

      /**
       * Get the paths.
       */
      const paths& get_paths () const;

      /**
       * Get the archive files.
       */
      void get_archive_files (files& afiles);

      /**
       * Get the object files including those in archives.
       */
      void get_object_files (files& ofiles);

      /**
       * Get the archive count.
       */
      int archive_count () const;

      /**
       * Get the object count.
       */
      int object_count () const;

      /**
       * Get the path count.
       */
      int path_count () const;

      /**
       * Output archive files.
       */
      void output_archive_files (std::ostream& out);

      /**
       * Output archive files.
       */
      void output_object_files (std::ostream& out);

    protected:

      /**
       * Input a path into the cache.
       */
      virtual void input (const std::string& path);

    private:
      paths    paths_;    //< The names of the files to process.
      archives archives_; //< The archive files.
      objects  objects_;  //< The object files.
      bool     opened;    //< The cache is open.
    };

    /**
     * Find the libraries given the list of libraries as bare name which
     * have 'lib' and '.a' added.
     */
    void find_libraries (paths& libraries, paths& libpaths, paths& libs);

  }
}

#endif
