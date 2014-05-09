
/*! @file ConfigFile.cc
 *  @brief ConfigFile Implementation
 *
 *  This file contains the implementation of the FileReader class.
 */

#include "ConfigFile.h"
#include <string.h>
#include <stdio.h>
#include <ctype.h>

namespace Configuration {

  FileReader::FileReader(
    Options_t *options
  )
  {
    options_m = options;
  }

  FileReader::~FileReader()
  {
  }

  bool FileReader::processFile(
    const char* const     file
  )
  {
    #define METHOD "FileReader::processFile - "
    FILE *  in;
    char    line[256];
    char    option[256];
    char    value[256];
    int     line_no;
    int     i;
    int     j;

    if ( file == NULL ) {
      fprintf( stderr, METHOD "NULL filename\n" );
      return false;
    }

    in = fopen( file, "r" );
    if ( !in ) {
      fprintf( stderr, METHOD "unable to open %s\n", file );
      return false;
    }

    line_no = 0;
    while (fgets(line, sizeof(line), in) != NULL) {
      int length;

      line_no++;

      length = (int) strlen( line );
      if ( line[length - 1] != '\n' ) {
        fprintf(
          stderr,
          "%s: line %d is too long",
          file,
          line_no
        );
        continue;
      }

      line[length - 1] = '\0';
      length--;

      /*
       *  Strip off comments at end of line
       *
       *      LHS = RHS   # comment
       */
      for (i=0 ; i<length ; i++ ) {
        if ( line[i] == '#' ) {
          line[i] = '\0';
          length = i;
          break;
        }
      }

      /*
       *  Strip off trailing white space
       */
      for (i=length-1 ; i>=0 && isspace(line[i]) ; i-- )
        ;

      line[i+1] = '\0';
      length = i+1;

      /* Ignore empty lines.  We have stripped
       * all comments and blanks therefore, only
       * an empty string needs to be checked.
       */
      if (line[0] == '\0') 
        continue;

      if (sscanf(line, "%s", option) != 1) {
        fprintf(
          stderr,
          "%s: line %d is invalid: %s\n",
          file,
          line_no,
          line
        );
        continue;
      }

      for (i=0; ((line[i] != '=') && (i<length)); i++)
        ;

      if (i == length) {
        fprintf(
          stderr,
          "%s: line %d is invalid: %s\n",
          file,
          line_no,
          line
        );
        continue;
      }

      i++;
      value[0] = '\0';
      while ( isspace(line[i]) )
        i++;
      for (j=0; line[i] != '\0'; i++, j++ )
        value[j] = line[i];
      value[j] = '\0'; 
      if (value[0] == '\0') {
        fprintf(
          stderr,
          "%s: line %d is invalid: %s\n",
          file,
          line_no,
          line
        );
        continue;
      }

      if ( !setOption(option, value) ) {
        fprintf(
          stderr,
          "%s: line %d: option %s is unknown\n",
          file,
          line_no,
          option
        );
        continue;
      }

    }

    return false;
  }

  bool FileReader::setOption(
    const char* const option,
    const char* const value
  )
  {
    Options_t *o;

    for ( o=options_m ; o->option ; o++ ) {
      if ( !strcmp( o->option, option ) ) {
        o->value = strdup( value );
        return true;
      }
    }
    return false;
  }

  const char *FileReader::getOption(
    const char* const option
  )
  {
    Options_t *o;

    for ( o=options_m ; o->option ; o++ ) {
      if ( !strcmp( o->option, option ) ) {
        return o->value;
      }
    }
    return NULL;
  }

  void FileReader::printOptions(void)
  {
    Options_t *o;

    for ( o=options_m ; o->option ; o++ ) {
      fprintf( stderr, "(%s)=(%s)\n", o->option, o->value );
    }
  }
}
