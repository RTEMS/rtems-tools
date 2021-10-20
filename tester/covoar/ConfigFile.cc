
/*! @file ConfigFile.cc
 *  @brief ConfigFile Implementation
 *
 *  This file contains the implementation of the FileReader class.
 */

#include "ConfigFile.h"
#include <string.h>
#include <ctype.h>

#include <iostream>
#include <fstream>
#include <sstream>

static void print_invalid_line_number( const std::string& file, int line_no )
{
  std::cerr << file << ": line" << line_no << " is invalid: " << line
            << std::endl;
}

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
    const std::string&     file
  )
  {
    #define METHOD "FileReader::processFile - "
    #define MAX_LENGTH 256
    std::ifstream in;
    std::string line;
    char option[MAX_LENGTH];
    char value[MAX_LENGTH];
    int     line_no;
    int     i;
    int     j;

    if ( file.empty() ) {
      std::cerr << METHOD << "Empty filename" << std::endl;
      return false;
    }

    in.open( file );
    if ( !in.is_open() ) {
      std::cerr << METHOD << "unable to open " << file << std::endl;
      return false;
    }

    line_no = 0;
    while ( std::getline( line, MAX_LENGTH ) ) {
      int length;

      line_no++;

      length = (int) line.length();
      if ( length > MAX_LENGTH ) {
        std::cerr << file << ": line " << line_no << " is too long"
                  << std::endl;
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

      if (std::sscanf(line.c_str(), "%s", option) != 1) {
        print_invalid_line_number(file, line_no);
        continue;
      }

      for (i=0; ((line[i] != '=') && (i<length)); i++)
        ;

      if (i == length) {
        print_invalid_line_number(file, line_no);
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
        print_invalid_line_number(file, line_no);
        continue;
      }

      if ( !setOption(option, value) ) {
        print_invalid_line_number(file, line_no);
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
      std::cerr << '(' << o->option << ")=(" << o->value << ')' << std::endl;
    }
  }
}
