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
#include <rld-path.h>

namespace rld
{
  namespace files
  {
    /**
     * Container of files.
     */
    typedef std::vector < file > files;

    /**
     * Container of archive files.
     */
    typedef std::map < const std::string, archive* > archives;

    /**
     * Container of object files.
     */
    typedef std::map < const std::string, object* > objects;

    /**
     * Container list of object files.
     */
    typedef std::list < object* > object_list;

    /**
     * Byte order of the image.
     */
    enum byteorder
    {
      big_endian,
      little_endian
    };

    /**
     * A file is a single object file that is either in an @ref archive or
     * a separate stand alone @ref object file.
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
       *
       * @return const std::string& The archive name.
       */
      const std::string& aname () const;

      /**
       * The object name. There is always an object name.
       *
       * @return const std::string& The object name.
       */
      const std::string& oname () const;

      /**
       * The object's offset in the archive or on disk.
       *
       * @return off_t The offset part of the file name.
       */
      off_t offset () const;

      /**
       * The object's size in the archive.
       *
       * @return size_t The size of the file in bytes.
       */
      size_t size () const;

    private:
      std::string aname_;  //< The archive name.
      std::string oname_;  //< The object name.
      off_t       offset_; //< The object's offset in the archive.
      size_t      size_;   //< The object's size in the archive or on disk.
    };

    /**
     * Image is the base file type and lets us have a single container to hold
     * the types of images we need to support.
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
       * @param path The file path.
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
       *
       * @param name The @ref file name.
       */
      virtual void open (file& name);

      /**
       * Open the image. You can open the image more than once but you need to
       * close it the same number of times.
       *
       * @param writeable If true the image is open as writable. The default is
       *                  false.
       */
      virtual void open (bool writable = false);

      /**
       * Close the image.
       */
      virtual void close ();

      /**
       * Read a block from the file.
       *
       * @param buffer The buffer to read the data into.
       * @param size The amount of data to read.
       * @return ssize_t The amount of data read.
       */
      virtual ssize_t read (void* buffer, size_t size);

      /**
       * Write a block from the file.
       *
       * @param buffer The buffer of data to write.
       * @param size The amount of data to write.
       * @return ssize_t The amount of data written.
       */
      virtual ssize_t write (const void* buffer, size_t size);

      /**
       * Seek to the offset in the image.
       *
       * @param offset The offset to seek too.
       */
      virtual void seek (off_t offset);

      /**
       * Seek and then read.
       *
       * @param offset The offset to seek too before reading.
       * @param buffer The buffer to read the data into.
       * @param size The amount of data to read.
       * @retval true The data requested was read.
       * @retval false The request data was not read.
       */
      virtual bool seek_read (off_t offset, uint8_t* buffer, size_t size);

      /**
       * Seek and then write.
       *
       * @param offset The offset to seek too before writing.
       * @param buffer The buffer of data to write.
       * @param size The amount of data to write.
       * @retval true The data requested was written.
       * @retval false The request data was not written.
       */
      virtual bool seek_write (off_t offset, const void* buffer, size_t size);

      /**
       * The name of the image.
       *
       * @param const file& The @ref file name of the image.
       */
      const file& name () const;

      /**
       * References to the image.
       *
       * @return int The number of references the image has.
       */
      virtual int references () const;

      /**
       * The file size.
       *
       * @return size_t The size of the image.
       */
      virtual size_t size () const;

      /**
       * The file descriptor.
       *
       * @return int The operating system file descriptor handle.
       */
      virtual int fd () const;

      /**
       * The ELF reference.
       *
       * @return elf::file& The @ref elf::file object of the image.
       */
      elf::file& elf ();

      /**
       * Return the image's byte order.
       */
      byteorder get_byteorder () const;

      /**
       * A symbol in the image has been referenced.
       */
      virtual void symbol_referenced ();

      /**
       * Return the number of symbol references.
       *
       * @return int The symbol references count.
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
     *
     * @param in The input image.
     * @param out The output image.
     * @param size The amouint of data to copy.
     */
    void copy (image& in, image& out, size_t size);

    /**
     * The archive class proivdes access to object files that are held in a AR
     * format file. GNU AR extensions are supported. The archive is a kind of
     * @ref image and provides the container for the @ref object's that it
     * contains.
     */
    class archive:
      public image
    {
    public:
      /**
       * Open a archive format file that contains ELF object files.
       *
       * @param name The name of the @ref archive.
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
       * @retval true The name matches.
       * @retval false The name does not match.
       */
      bool is (const std::string& name) const;

      /**
       * Check this is a valid archive.
       *
       * @retval true It is a valid archive.
       * @retval false It is not a valid archive.
       */
      bool is_valid ();

      /**
       * Load @ref object's from the @ref archive adding each to the provided
       * @ref objects container.
       *
       * @param objs The container the loaded object files are added too.
       */
      void load_objects (objects& objs);

      /**
       * Get the name.
       *
       * @return const std::string& Return a reference to the archive's name.
       */
      const std::string& get_name () const;

      /**
       * Less than operator for the map container. It compares the name of the
       * the @ref archive.
       *
       * @param rhs The right hand side of the '<' operator.
       * @return true The right hand side is less than this archive.
       * @return false The right hand side is greater than or equal to this
       *               archive.
       */
      bool operator< (const archive& rhs) const;

      /**
       * Create a new archive containing the given set of objects. If
       * referening an existing archive it is overwritten.
       *
       * @param objects The list of objects to place in the archive.
       */
      void create (object_list& objects);

    private:

      /**
       * Read the archive header and check the magic number is valid.
       *
       * @param offset The offset in the file to read the header.
       * @param header Read the header into here. There must be enough space.
       * @retval true The header was read successfull and the magic number
       *               matched.
       * @retval false The header could not be read from the @ref image.
       */
      bool read_header (off_t offset, uint8_t* header);

      /**
       * Add the object file from the archive to the object's container.
       *
       * @param objs The container to add the object to.
       * @param name The name of the object file being added.
       * @param offset The offset in the @ref archive of the object file.
       * @param size The size of the object file.
       */
      void add_object (objects&    objs,
                       const char* name,
                       off_t       offset,
                       size_t      size);

      /**
       * Write a file header into the archive.
       *
       * @param name The name of the archive.
       * @param mtime The modified time of the archive.
       * @param uid The user id of the archive.
       * @param gid The group id of the archive.
       * @param mode The mode of the archive.
       * @param size The size of the archive.
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
       * Cannot assign using the assignment operator.
       */
      archive& operator= (const archive& rhs);
    };

    /**
     * A relocation record. We extract what we want because the elf::section
     * class requires the image be left open as references are alive. We
     * extract and keep the data we need to create the image.
     */
    struct relocation
    {
      const uint32_t    offset;    //< The section offset.
      const uint32_t    type;      //< The type of relocation record.
      const uint32_t    info;      //< The ELF info field.
      const int32_t     addend;    //< The constant addend.
      const std::string symname;   //< The name of the symbol.
      const uint32_t    symtype;   //< The type of symbol.
      const int         symsect;   //< The symbol's section symbol.
      const uint32_t    symvalue;  //< The symbol's value.
      const uint32_t    symbinding;//< The symbol's binding.

      /**
       * Construct from an ELF relocation record.
       */
      relocation (const elf::relocation& er);

    private:
      /**
       * The default constructor is not allowed due to all elements being
       * const.
       */
      relocation ();
    };

    /**
     * A container of relocations.
     */
    typedef std::list < relocation > relocations;

    /**
     * The sections attributes. We extract what we want because the
     * elf::section class requires the image be left open as references are
     * alive. We extract and keep the data we need to create the image.
     */
    struct section
    {
      const std::string name;      //< The name of the section.
      const int         index;     //< The section's index in the object file.
      const uint32_t    type;      //< The type of section.
      const size_t      size;      //< The size of the section.
      const uint32_t    alignment; //< The alignment of the section.
      const uint32_t    link;      //< The ELF link field.
      const uint32_t    info;      //< The ELF info field.
      const uint32_t    flags;     //< The ELF flags.
      const off_t       offset;    //< The ELF file offset.
      const uint64_t    address;   //< The ELF address.
      bool              rela;      //< Relocation records have the addend field.
      relocations       relocs;    //< The sections relocations.

      /**
       * Construct from an ELF section.
       *
       * @param es The ELF section to load the object file section from.
       */
      section (const elf::section& es);

      /**
       * Load the ELF relocations.
       *
       * @param es The ELF section to load the relocations from.
       */
      void load_relocations (const elf::section& es);

    private:
      /**
       * The default constructor is not allowed due to all elements being
       * const.
       */
      section ();
    };

    /**
     * A container of sections.
     */
    typedef std::list < section > sections;

    /**
     * Sum the sizes of a container of sections.
     */
    size_t sum_sizes (const sections& secs);

    /**
     * Find the section that matches the index in the sections provided.
     */
    const section* find (const sections& secs, const int index);

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
       * Begin the object file session.
       */
      void begin ();

      /**
       * End the object file session.
       */
      void end ();

      /**
       * If valid returns true the begin has been called and the object has
       * been validated as being in a suitable format.
       */
      bool valid () const;

      /**
       * Load the symbols into the symbols table.
       *
       * @param symbols The symbol table to load.
       * @param local Include local symbols. The default is not to.
       */
      void load_symbols (symbols::table& symbols, bool local = false);

      /**
       * Load the relocations.
       */
      void load_relocations ();

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
      symbols::symtab& unresolved_symbols ();

      /**
       * Return the list external symbols.
       */
      symbols::pointers& external_symbols ();

      /**
       * Return a container sections that match the requested type and
       * flags. The filtered section container is not cleared so any matching
       * sections are appended.
       *
       * @param filtered_secs The container of the matching sections.
       * @param type The section type. Must match. If 0 matches any.
       * @param flags_in The sections flags that must be set. This is a
       *                 mask. If 0 matches any.
       * @param flags_out The sections flags that must be clear. This is a
       *                 mask. If 0 this value is ignored.
       */
      void get_sections (sections& filtered_secs,
                         uint32_t  type = 0,
                         uint64_t  flags_in = 0,
                         uint64_t  flags_out = 0);

      /**
       * Return a container sections that match the requested name. The
       * filtered section container is not cleared so any matching sections are
       * appended.
       *
       * @param filtered_secs The container of the matching sections.
       * @param name The name of the section.
       */
      void get_sections (sections& filtered_secs, const std::string& name);

      /**
       * Get a section given an index number.
       *
       * @param index The section index to search for.
       */
      const section& get_section (int index) const;

      /**
       * Set the object file's resolving flag.
       */
      void resolve_set ();

      /**
       * Clear the object file's resolving flag.
       */
      void resolve_clear ();

      /**
       * The resolving state.
       */
      bool resolving () const;

      /**
       * Set the object file resolved flag.
       */
      void resolved_set ();

      /**
       * The resolved state.
       */
      bool resolved () const;

    private:
      archive*          archive_;   //< Points to the archive if part of an
                                    //  archive.
      bool              valid_;     //< If true begin has run and finished.
      symbols::symtab   unresolved; //< This object's unresolved symbols.
      symbols::pointers externals;  //< This object's external symbols.
      sections          secs;       //< The sections.
      bool              resolving_; //< The object is being resolved.
      bool              resolved_;  //< The object has been resolved.

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
      void add (path::paths& paths__);

      /**
       * Add a container of path to the cache.
       */
      void add_libraries (path::paths& paths__);

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
       * @param locals Include local symbols. The default does not include them.
       */
      void load_symbols (symbols::table& symbols, bool locals = false);

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
      void get_objects (object_list& list) const;

      /**
       * Get the paths.
       */
      const path::paths& get_paths () const;

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
      path::paths paths_;    //< The names of the files to process.
      archives    archives_; //< The archive files.
      objects     objects_;  //< The object files.
      bool        opened;    //< The cache is open.
    };

    /**
     * Copy the in file to the out file.
     *
     * @param in The input file.
     * @param out The output file.
     * @param size The amount to copy. If 0 the whole on in is copied.
     */
    void copy_file (image& in, image& out, size_t size = 0);

    /**
     * Find the libraries given the list of libraries as bare name which
     * have 'lib' and '.a' added.
     */
    void find_libraries (path::paths& libraries,
                         path::paths& libpaths,
                         path::paths& libs);

  }
}

#endif
