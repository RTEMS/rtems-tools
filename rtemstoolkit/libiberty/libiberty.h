/* Function declarations for libiberty.

   Copyright 1997, 1998, 1999, 2000, 2001, 2002, 2003, 2004, 2005,
   2006, 2007, 2008, 2009, 2010 Free Software Foundation, Inc.
   
   Note - certain prototypes declared in this header file are for
   functions whoes implementation copyright does not belong to the
   FSF.  Those prototypes are present in this file for reference
   purposes only and their presence in this file should not construed
   as an indication of ownership by the FSF of the implementation of
   those functions in any way or form whatsoever.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street - Fifth Floor,
   Boston, MA 02110-1301, USA.
   
   Written by Cygnus Support, 1994.

   The libiberty library provides a number of functions which are
   missing on some operating systems.  We do not declare those here,
   to avoid conflicts with the system header files on operating
   systems that do support those functions.  In this file we only
   declare those functions which are specific to libiberty. 

  Hacked up libiberty.h file from the real one.
*/

#if !defined (_LIBIERTY_H_)
#define _LIBIERTY_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <ansidecl.h>
#include <stdlib.h>
#include <stdio.h>

#define ARRAY_SIZE(a) (sizeof (a) / sizeof ((a)[0]))

#define xstrdup   strdup
#define xstrerror strerror

/* These macros provide a K&R/C89/C++-friendly way of allocating structures
   with nice encapsulation.  The XDELETE*() macros are technically
   superfluous, but provided here for symmetry.  Using them consistently
   makes it easier to update client code to use different allocators such
   as new/delete and new[]/delete[].  */

/* Scalar allocators.  */

#define XALLOCA(T)              ((T *) alloca (sizeof (T)))
#define XNEW(T)                 ((T *) malloc (sizeof (T)))
#define XCNEW(T)                ((T *) calloc (1, sizeof (T)))
#define XDUP(T, P)              ((T *) memdup ((P), sizeof (T), sizeof (T)))
#define XDELETE(P)              free ((void*) (P))

/* Array allocators.  */

#define XALLOCAVEC(T, N)        ((T *) alloca (sizeof (T) * (N)))
#define XNEWVEC(T, N)           ((T *) malloc (sizeof (T) * (N)))
#define XCNEWVEC(T, N)          ((T *) calloc ((N), sizeof (T)))
#define XDUPVEC(T, P, N)        ((T *) memdup ((P), sizeof (T) * (N), sizeof (T) * (N)))
#define XRESIZEVEC(T, P, N)     ((T *) realloc ((void *) (P), sizeof (T) * (N)))
#define XDELETEVEC(P)           free ((void*) (P))

/* Allocators for variable-sized structures and raw buffers.  */

#define XALLOCAVAR(T, S)        ((T *) alloca ((S)))
#define XNEWVAR(T, S)           ((T *) malloc ((S)))
#define XCNEWVAR(T, S)          ((T *) calloc (1, (S)))
#define XDUPVAR(T, P, S1, S2)   ((T *) memdup ((P), (S1), (S2)))
#define XRESIZEVAR(T, P, S)     ((T *) realloc ((P), (S)))

/* Concatenate an arbitrary number of strings.  You must pass NULL as
   the last argument of this function, to terminate the list of
   strings.  Allocates memory using xmalloc.  */

extern char *concat (const char *, ...) ATTRIBUTE_MALLOC ATTRIBUTE_SENTINEL;

/* Concatenate an arbitrary number of strings.  You must pass NULL as
   the last argument of this function, to terminate the list of
   strings.  Allocates memory using xmalloc.  The first argument is
   not one of the strings to be concatenated, but if not NULL is a
   pointer to be freed after the new string is created, similar to the
   way xrealloc works.  */

extern char *reconcat (char *, const char *, ...) ATTRIBUTE_MALLOC ATTRIBUTE_SENTINEL;

/* Determine the length of concatenating an arbitrary number of
   strings.  You must pass NULL as the last argument of this function,
   to terminate the list of strings.  */

extern unsigned long concat_length (const char *, ...) ATTRIBUTE_SENTINEL;

/* Concatenate an arbitrary number of strings into a SUPPLIED area of
   memory.  You must pass NULL as the last argument of this function,
   to terminate the list of strings.  The supplied memory is assumed
   to be large enough.  */

extern char *concat_copy (char *, const char *, ...) ATTRIBUTE_SENTINEL;

/* Concatenate an arbitrary number of strings into a GLOBAL area of
   memory.  You must pass NULL as the last argument of this function,
   to terminate the list of strings.  The supplied memory is assumed
   to be large enough.  */

extern char *concat_copy2 (const char *, ...) ATTRIBUTE_SENTINEL;

/* This is the global area used by concat_copy2.  */

extern char *libiberty_concat_ptr;

/* Return a temporary file name or NULL if unable to create one.  */

extern char *make_temp_file (const char *) ATTRIBUTE_MALLOC;

/* Flags for pex_init.  These are bits to be or'ed together.  */

/* Record subprocess times, if possible.  */
#define PEX_RECORD_TIMES        0x1

/* Use pipes for communication between processes, if possible.  */
#define PEX_USE_PIPES           0x2

/* Save files used for communication between processes.  */
#define PEX_SAVE_TEMPS          0x4

/* Prepare to execute one or more programs, with standard output of
   each program fed to standard input of the next.
   FLAGS        As above.
   PNAME        The name of the program to report in error messages.
   TEMPBASE     A base name to use for temporary files; may be NULL to
                use a random name.
                Returns NULL on error.  */

extern struct pex_obj *pex_init (int flags, const char *pname,
                                 const char *tempbase);

/* Flags for pex_run.  These are bits to be or'ed together.  */

/* Last program in pipeline.  Standard output of program goes to
   OUTNAME, or, if OUTNAME is NULL, to standard output of caller.  Do
   not set this if you want to call pex_read_output.  After this is
   set, pex_run may no longer be called with the same struct
   pex_obj.  */
#define PEX_LAST                0x1

/* Search for program in executable search path.  */
#define PEX_SEARCH              0x2

/* OUTNAME is a suffix.  */
#define PEX_SUFFIX              0x4

/* Send program's standard error to standard output.  */
#define PEX_STDERR_TO_STDOUT    0x8

/* Input file should be opened in binary mode.  This flag is ignored
   on Unix.  */
#define PEX_BINARY_INPUT        0x10

/* Output file should be opened in binary mode.  This flag is ignored
   on Unix.  For proper behaviour PEX_BINARY_INPUT and
   PEX_BINARY_OUTPUT have to match appropriately--i.e., a call using
   PEX_BINARY_OUTPUT should be followed by a call using
   PEX_BINARY_INPUT.  */
#define PEX_BINARY_OUTPUT       0x20

/* Capture stderr to a pipe.  The output can be read by
   calling pex_read_err and reading from the returned
   FILE object.  This flag may be specified only for
   the last program in a pipeline.  

   This flag is supported only on Unix and Windows.  */
#define PEX_STDERR_TO_PIPE      0x40

/* Capture stderr in binary mode.  This flag is ignored
   on Unix.  */
#define PEX_BINARY_ERROR        0x80


/* Execute one program.  Returns NULL on success.  On error returns an
   error string (typically just the name of a system call); the error
   string is statically allocated.

   OBJ		Returned by pex_init.

   FLAGS	As above.

   EXECUTABLE	The program to execute.

   ARGV		NULL terminated array of arguments to pass to the program.

   OUTNAME	Sets the output file name as follows:

		PEX_SUFFIX set (OUTNAME may not be NULL):
		  TEMPBASE parameter to pex_init not NULL:
		    Output file name is the concatenation of TEMPBASE
		    and OUTNAME.
		  TEMPBASE is NULL:
		    Output file name is a random file name ending in
		    OUTNAME.
		PEX_SUFFIX not set:
		  OUTNAME not NULL:
		    Output file name is OUTNAME.
		  OUTNAME NULL, TEMPBASE not NULL:
		    Output file name is randomly chosen using
		    TEMPBASE.
		  OUTNAME NULL, TEMPBASE NULL:
		    Output file name is randomly chosen.

		If PEX_LAST is not set, the output file name is the
   		name to use for a temporary file holding stdout, if
   		any (there will not be a file if PEX_USE_PIPES is set
   		and the system supports pipes).  If a file is used, it
   		will be removed when no longer needed unless
   		PEX_SAVE_TEMPS is set.

		If PEX_LAST is set, and OUTNAME is not NULL, standard
   		output is written to the output file name.  The file
   		will not be removed.  If PEX_LAST and PEX_SUFFIX are
   		both set, TEMPBASE may not be NULL.

   ERRNAME	If not NULL, this is the name of a file to which
		standard error is written.  If NULL, standard error of
		the program is standard error of the caller.

   ERR		On an error return, *ERR is set to an errno value, or
   		to 0 if there is no relevant errno.
*/

extern const char *pex_run (struct pex_obj *obj, int flags,
			    const char *executable, char * const *argv,
			    const char *outname, const char *errname,
			    int *err);

/* As for pex_run (), but takes an extra parameter to enable the
   environment for the child process to be specified.

   ENV		The environment for the child process, specified as
		an array of character pointers.  Each element of the
		array should point to a string of the form VAR=VALUE,
                with the exception of the last element which must be
                a null pointer.
*/

extern const char *pex_run_in_environment (struct pex_obj *obj, int flags,
			                   const char *executable,
                                           char * const *argv,
                                           char * const *env,
              	          		   const char *outname,
					   const char *errname, int *err);

/* Return a stream for a temporary file to pass to the first program
   in the pipeline as input.  The file name is chosen as for pex_run.
   pex_run closes the file automatically; don't close it yourself.  */

extern FILE *pex_input_file (struct pex_obj *obj, int flags,
                             const char *in_name);

/* Return a stream for a pipe connected to the standard input of the
   first program in the pipeline.  You must have passed
   `PEX_USE_PIPES' to `pex_init'.  Close the returned stream
   yourself.  */

extern FILE *pex_input_pipe (struct pex_obj *obj, int binary);

/* Read the standard output of the last program to be executed.
   pex_run can not be called after this.  BINARY should be non-zero if
   the file should be opened in binary mode; this is ignored on Unix.
   Returns NULL on error.  Don't call fclose on the returned FILE; it
   will be closed by pex_free.  */

extern FILE *pex_read_output (struct pex_obj *, int binary);

/* Read the standard error of the last program to be executed.
   pex_run can not be called after this.  BINARY should be non-zero if
   the file should be opened in binary mode; this is ignored on Unix.
   Returns NULL on error.  Don't call fclose on the returned FILE; it
   will be closed by pex_free.  */

extern FILE *pex_read_err (struct pex_obj *, int binary);

/* Return exit status of all programs in VECTOR.  COUNT indicates the
   size of VECTOR.  The status codes in the vector are in the order of
   the calls to pex_run.  Returns 0 on error, 1 on success.  */

extern int pex_get_status (struct pex_obj *, int count, int *vector);

/* Return times of all programs in VECTOR.  COUNT indicates the size
   of VECTOR.  struct pex_time is really just struct timeval, but that
   is not portable to all systems.  Returns 0 on error, 1 on
   success.  */

struct pex_time
{
  unsigned long user_seconds;
  unsigned long user_microseconds;
  unsigned long system_seconds;
  unsigned long system_microseconds;
};

extern int pex_get_times (struct pex_obj *, int count,
			  struct pex_time *vector);

/* Clean up a pex_obj.  If you have not called pex_get_times or
   pex_get_status, this will try to kill the subprocesses.  */

extern void pex_free (struct pex_obj *);

/* Just execute one program.  Return value is as for pex_run.
   FLAGS	Combination of PEX_SEARCH and PEX_STDERR_TO_STDOUT.
   EXECUTABLE	As for pex_run.
   ARGV		As for pex_run.
   PNAME	As for pex_init.
   OUTNAME	As for pex_run when PEX_LAST is set.
   ERRNAME	As for pex_run.
   STATUS	Set to exit status on success.
   ERR		As for pex_run.
*/

extern const char *pex_one (int flags, const char *executable,
			    char * const *argv, const char *pname,
			    const char *outname, const char *errname,
			    int *status, int *err);

#ifdef __cplusplus
}
#endif

#endif
