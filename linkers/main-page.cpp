/*
 * Copyright (c) 2011-2013, Chris Johns <chrisj@rtems.org>
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
 * @mainpage RTEMS Linker Tools
 *
 * The RTEMS Linker is a suite of tools that create and manage @subpage rtems-apps
 * that are dynamically loadable by the @subpage rtems-rtl on target
 * hardware. The target code uses the standard `dlopen`, `dlclose` type calls
 * to load and manage modules, object files or archives on the target at
 * runtime. The RTEMS Linker forms a part of this process by helping managing
 * the object files, libraries and applications on a host machine. This host
 * processing simplifies the demands on the target and avoids wastefull excess
 * of files and data that may not be used at runtime.
 *
 * These tools are written in C++ with some 3rd party packages in C. The
 * license for this RTEMS Tools code is a BSD type open source license. The
 * package includes code from:
 *
 * -# @b efltoolchain - http://sourceforge.net/apps/trac/elftoolchain/
 * -# @b libiberty - Libiberty code from GCC (GPL)
 * -# @b fastlz - http://fastlz.org/
 *
 * The project uses a C++ demangler and PEX code from the GCC project. This
 * code is GPL making this project GPL. A platform independent way to execute
 * sub-processes and capture the output that is not GPL is most welcome.
 *
 * @subpage build-me details building this package with @subpage waf.
 *
 * The tools provided are:
 *
 * - @subpage rtems-ld
 * - @subpage rtems-syms
 * - @subpage rtems-rap
 *
 * ____________________________________________________________________________
 * @copyright
 * Copyright (c) 2011-2013, Chris Johns <chrisj@rtems.org>
 * @copyright
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 * @copyright
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */

/**
 * @page rtems-apps RTEMS Applications
 *
 * The RTEMS Linker and @ref rtems-rtl provides RTEMS with the ability to
 * support applications loaded and linked at runtime. RTEMS is a single address
 * space real-time operating system designed for embedded systems that are
 * statically linked therefore the idea of applications requires some extra
 * understanding when applied to RTEMS. They are not essential, rather they are
 * important in a range of systems that have the resources available to support
 * them.
 *
 * Applications allow:
 *
 * - A team to create a single verified base kernel image that is used by all
 *   team developers. This kernel could be embedded on the target hardware and
 *   applications loaded over a network. The verified kernel binary used during
 *   development can be shipped without being changed.
 *
 * - Layered applications designed as modules that are loaded at runtime to
 *   create a specific target environment for a specific system. This approach
 *   allows development of modules that become verified components. An example
 *   is the NASA Core Flight Executive.
 *
 * - Runtime configuration and loading of features or drivers based on
 *   configuration data or detected hardware. This is important if your target
 *   hardware has an external bus such as PCI. You can add a new driver to a
 *   system without needing to rebuild the kernel and application lowering the
 *   verify and validation costs. If these are high the savings can be
 *   substantial.
 *
 * RTEMS is a single address space operating system therefore any code loaded
 * is loaded into that address space. This means applications are not operating
 * in a separate protected address space you typically get with host type
 * operating systems. You need to control and manage what you allow to load on
 * your system. This is no differerent to how single image RTEMS are currently
 * created and managed. The point being RTEMS applications only changes the way
 * you package and maintain your applications and do not provide any improved
 * security or protection. You need to do this as your currently do with
 * testing and careful design.
 *
 * RTEMS is statically linked to a fixed address and does not support dynamic
 * ELF files. Dynamic ELF files are designed for use in virtual memory
 * protected address space operating systems. They contain Position Independent
 * Code (PIC) code, Procedure Linkage Tables (PLT) and Global Offset Tables
 * (GOT) and are effective in not only allowing dynamic linking at runtime but
 * also the sharing of the code between separate process address spaces. Using
 * virtual memory and a memory management unit, a protected address space
 * operating system can efficiently share code between processes with minimal
 * performance overhead. RTEMS has no such need because it is a single address
 * space and all code is shared therefore ELF dynamic files only add complexity
 * and performance overhead. This means RTEMS needs a target based run-time
 * link editor that can relocate and fix up static code when loading it and
 * RTEMS loadable files need to contain the symbols and relocation records to
 * allow relocation to happen.
 *
 * The @ref rtems-rtl supports the followiing file formats:
 *
 * -# Relocatable ELF (ELF)
 * -# RTEMS Application (RAP)
 * -# Archive (AR) Libraries with GNU extensions
 *
 * ### Relocation ELF Files
 *
 * The @ref rtems-rtl can load standard relocatable ELF format files. They can
 * be stripped or unstripped. This ELF file is the standard output from the
 * compiler and is contained in the standard libraries.
 *
 * ### RTEMS Application (RAP) Files.
 *
 * The @ref rtems-rtl can load RAP format files. This format is RTEMS specific
 * and is designed to minimise the overhead and resources needed to load the
 * file on the target. A RAP file is compressed using LZ77 compression and
 * contains only the following sections:
 *
 * - `.text` - The executable code section.
 * - `.const` - The constants and strings section.
 * - `.ctor` - The constructor table section.
 * - `.dtor` - The destructor table section.
 * - `.data` - The initialised data section.
 *
 * The `.bss` uninitialised data section is only a size. A RAP file also
 * contains a symbol string table and symbol table that are directly loadable
 * into into the target memory. Finally the RAP contains the relocation
 * records. The format is structured so it can be read and processed as a
 * stream with the need to seek on the file.
 *
 * The @ref rtems-ld can output RAP format files suitable for loading. It will
 * take the object files from the command line and the referenced files from
 * the libraries and merge all the sections, symbols and relocation records to
 * create the RAP format file.
 *
 * RAP format files are the most efficient way to load applications or modules
 * because all object files are merged into an single image. Each file loaded
 * on the target has and overhead therefore lowering the number of files loaded
 * lowers the overhead. You could also use the standard linker to incrementally
 * link the command line object files to archieve the same effect.
 *
 * ### Archive (AR) Library Files
 *
 * The @ref rtems-rtl can load from archive or library type files. The file
 * name syntax lets a user reference a file in an archive. The format is:
 *
 * @par
 * `libme.a:foo.o@12345`
 *
 * where `libme.a` is the archive file name, `foo.o` is the file in the archive
 * and `@12345` is optionally the offset in the archive where the file
 * starts. The optional offset helps speed up load by avoiding the file name
 * table search. If the archive is stable and known the offset will be
 * fixed. If the file is located at the offset the file name table is searched.
 *
 * At this point in time only ELF files can be loaded from archives. Loading of
 * RAP format files is planned.
 *
 * ## An Application
 *
 * Applications are created the same way you create standard host type
 * programs. You compile the source files and link them using the @ref
 * rtems-ld.
 *
 * @code
 * $ rtems-ld --base my-rtems foo.o bar.o -o my-app.rap -L /lib/path -lstuff
 * @endcode
 *
 * The command line of the @ref rtems-ld is similar to a standard linker with
 * some extra features specific to RTEMS. You provide a list of object files,
 * libraries and library paths plus you need to provide the RTEMS kernel image
 * you will use to load the application. The RTEMS kernel image provides the
 * symbols in the kernel to the linker. Errors will be generated if symbols are
 * not located.
 *
 * The linker can output a archive of ELF files, a RAP file for a text script
 * of files that need to be loaded.
 *
 * The script lets you load and link the application at runtime on the
 * target. You need to copy the libraries referenced to the target.
 *
 * If you break your application into separate modules and each module
 * references a symbol in a library that is not in the base image the linker
 * will include the object file containing the symbol into each application
 * module. This is only of concern for the RAP format because it merges the
 * object files together. With the archive and scripts the loader will not load
 * object files with duplicate symbols.
 *
 * @note In time the linker will gain an option to not pull object modules from
 *       libraries into the RAP file. Another option will be added that will
 *       copy referenced library object files into a target library all
 *       application modules can share.
 *
 * ## Linking
 *
 * The @ref rtems-ld places the command line object files in the output image
 * and any reference object files found in libraries. If a symbol is located in
 * the kernel base image it is not searched for in the libraries.
 *
 * The architecture is automatically detected by inspecting the first object
 * file passed on the command line. All future object files loaded must match
 * the architecture for an error is raised. The linker supports all
 * architectures in a single binrary. It is not like the GNU tools which are
 * specific to an architecture.
 *
 * The linker needs to be able to locate the C compiler for the architecture
 * being linked. The compiler can be in the path for a command line option can
 * explicitly set the compiler. The compiler is used to locate the standard
 * libraries such as the C library.
 *
 *
 */

/**
 * @page rtems-rtl RTEMS Target Link Editor
 *
 * The RTEMS Target link editor is a C module you link to the RTEMS kernel to
 * provide the `dlopen`, `dlclose` etc family of calls. This code is a stand
 * alone project:
 *
 * @par
 * http://git.rtems.org/chrisj/rtl.git
 */

/**
 * @page build-me Building The RTEMS Linker
 *
 * This package is written in C++ therefore you need a current working C++
 * compiler for your host. The GCC or clang compilers can be used and clang was
 * used during the development. The build system is @ref waf.
 *
 * -# Clone the git repository:
 * @code
 * $ git clone http://git.rtems.org/chrisj/rtl-host.git rtl-host.git
 * @endcode
 * -# Configure with the default C++ compiler, default options, and an install
 * prefix of `$HOME/development/rtems/4.11`:
 * @code
 * $ waf configure --prefix=$HOME/development/rtems/4.11
 * @endcode
 * With @ref waf you build in the source directory and the @ref waf script
 * (`wscript`) will create a host specific directory. On MacOS the output is in
 * `build-darwin`. If you clean the build tree by deleting this directly you
 * will need to run the configure stage again.
 * @note The nominal RTEMS prefix is `/opt/rtems-4.11` where `4.11` is the
 *       version of RTEMS you are building the tools for. If you are using
 *       RTEMS 4.10 or a different version please use that version number. I
 *       always work under my home directory and under the `development/rtems`
 *       tree and then use the version number.
 * -# Build the tools:
 * @code
 * $ waf
 * @endcode
 * -# Install the tools to the configured prefix:
 * @code
 * $ waf install
 * @endcode
 *
 * You will now have the tools contained in this package build and installed.
 *
 * At this stage of the project's development there are no tests. I am wondering
 * if this could be a suitable GSoC project.
 *
 * To build with `clang` use the documented @ref waf method:
 * @code
 * $ CC=clang waf configure --prefix=$HOME/development/rtems/4.11
 * @endcode
 *
 * You can add some extra options to @ref waf's configure to change the
 * configuration. The options are:
 * @code
 * --rtems-version=RTEMS_VERSION
 *                       Set the RTEMS version
 * --c-opts=C_OPTS       Set build options, default: -O2.
 * --show-commands       Print the commands as strings.
 * @endcode
 *
 * - @b --rtems-version Set the RTEMS version number.
 * Not used.
 * - @b --c-opts Set the C and C++ compiler flags the tools are built with. For
 * example to disable all optimization flags to allow better debugging do:
 * @code
 * $ waf configure --prefix=$HOME/development/rtems/4.11 --c-opts=
 * @endcode
 * - @b --show-commands Prints the command string used to the invoke the
 *   compiler or linker. @ref waf normally prints a summary type line.
 *
 */

/**
 * @page waf Waf
 *
 * It is best you install waf by just downloading it from the Waf project
 * website:
 *
 * @par
 * http://code.google.com/p/waf/
 *
 * Waf is a Python program so you will also need to have a current Python
 * version installed and in your path.
 *
 * I download the latest "run from writable folder" version named single waf
 * file from http://code.google.com/p/waf/downloads/list to `$HOME/bin` and
 * symlink it to `waf`. The directory `$HOME/bin` is in my path.
 *
 * @code
 * $ cd $HOME/bin
 * $ curl http://waf.googlecode.com/files/waf-1.7.9 > waf-1.7.9
 *   % Total    % Received % Xferd  Average Speed   Time    Time     Time  Current
 *                                  Dload  Upload   Total   Spent    Left  Speed
 * 100 90486  100 90486    0     0  39912      0  0:00:02  0:00:02 --:--:-- 79934
 * $ rm -f waf
 * $ chmod +x waf-1.7.9
 * $ ln -s waf-1.7.9 waf
 * $ ./waf --version
 * waf 1.7.9 (9e92489dbc008e4abae9c147b1d63b48296797c2)
 * @endcode
 */

/**
 * @page rtems-ld RTEMS Linker
 *
 * The RTEMS Linker is a single tool that lets you create applications. It is a
 * special kind of linker and does not perform all the functions found in a
 * normal linker. RAP format output performs a partial increment link.
 *
 * ## Command
 *
 * `rtems-ld [options] objects`
 *
 * ## Options
 *
 * - @e Help (@b -h @b --help): \n
 *   Print the command line help then exit.
 *
 * - @e Version (@b -V @b --version): \n
 *   Print the linker's version then exit.
 *
 * - @e Verbose (@b -v @b --verbose): \n
 *   Control the trace output level. The RTEMS linker is always built with
 *   trace logic. The more times this appears on the command the more detailed
 *   the output becomes. The amount of output can be large at higher levels.
 *
 * - @e Warnings (@b -w @--warn): \n
 *   Print warnings.
 *
 * - @e Map (@b -M @b --map): \n
 *   Generate map output to stdout.
 *
 * - @e Output (@b -o @b --output): \n
 *   Set the output file name.
 *
 * - @e Output @e Format (@b -O @b --out-format): \n
 *   Set the output format. The valid formats are:
 *    Format     | Description
 *    -----------|----------------------------------------
 *    @b rap     |RTEMS application (LZ77, single image)
 *    @b elf     |ELF application (script, ELF files)
 *    @b script  |Script format (list of object files)
 *    @b archive |Archive format (collection of ELF files)
 *
 * - @e Library @e Path (@b -L @b --lib-path): \n
 *   Add a library path. More than one path can be added with multiple library
 *   path options.
 *
 * - @e Library (@b -l @b --lib): \n
 *   Add a library. More than one library can be added with multiple library
 *   paths.
 *
 * - @e No @e Standard @e Libraries (@b -n @b --no-stdlibs): \n
 *   Do not search the standard libraries. The linker uses the architecture C
 *   compiler to locate the installed standard libraries and these are
 *   automatically searched. If this option is used the C compiler is not
 *   called and the libraries are not added to the search list.
 *
 * - @e Entry @e Point (@b -e @b --entry): \n
 *   Set the entry point. This is used with the RAP format and defaults to
 *   `rtems`. The entry point is called when a RAP file is loaded by the
 *   target RAP loader.
 *
 * - @e Define @e Symbol (@b -d @b --define): \n
 *   Add a symbol to the symbol table. More than one symbol can be added
 *   with multiple define options.
 *
 * - @e Undefined @e Symbol (@b -u @b --undefined): \n
 *   Add an undefined symbol to the undefined symbol list. More than one
 *   undefined symbol can be added with multiple undefined options. This
 *   options will pull specific code into the output image.
 *
 * - @e RTEMS @e Kernel (@b -b @b --base): \n
 *   Set the RTEMS kernel image. This is the ELF file of the RTEMS kernel
 *   that will load the output from the linker. The RTEMS kernel is the
 *   @e base module or image. The linker does not pull the symbol from a
 *   library if the symbol is found in the base module. The kernel will
 *   load the target symbol table with these symbols so they can be
 *   resolved at runtime.
 *
 * - @e Architecture @e C @e Compiler (@b -C @b --cc): \n
 *   Set the architecture's C compiler. This is used to find the standard
 *   libraries.
 *
 * - @e Tool @e Prefix (@b -E @b --exec-prefix): \n
 *   Set the tool prefix. The tool prefix is the architecture and this is
 *   normally automatically set by inspecting the first object file
 *   loaded. This option allows the automatic detection to be overridden.
 *
 * - @e Machine @e Architecture (@b -a @b --march): \n
 *   Set the machine architecture.
 *
 * - @e Machine @e CPU (@b -c @b --mcpu): \n
 *   Set the machine architecture's CPU.
 */

/**
 * @page rtems-syms RTEMS Symbols Utility
 *
 * The symbols tool lets you see symbols in various RTEMS support file formats.
 */

/**
 * @page rtems-rap RTEMS Application (RAP) Utility
 *
 * The symbols tool lets you see symbols in various RTEMS support file formats.
 */
