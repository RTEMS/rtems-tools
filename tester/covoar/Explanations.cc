/*! @file Explanations.cc
 *  @brief Explanations Implementation
 *
 *  This file contains the implementation of the functions
 *  which provide a base level of functionality of a Explanations.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <fstream>

#include <rld.h>

#include "Explanations.h"

namespace Coverage {

  Explanations::Explanations()
  {
  }

  Explanations::~Explanations()
  {
  }

  void Explanations::load( const std::string& explanations )
  {
    std::ifstream explain;
    Explanation   e;
    int           line = 1;

    if ( explanations.empty() ) {
      return;
    }

    explain.open( explanations );
    if ( !explain ) {
      std::ostringstream what;
      what << "Unable to open " << explanations;
      throw rld::error( what, "Explanations::load" );
    }

    std::string input_line;
    while ( 1 ) {
      // Read the starting line of this explanation and
      // skip blank lines between
      do {
        std::getline( explain, input_line );
        if ( explain.fail() ) {
          return;
        }

        line++;
      } while ( input_line.empty() );

      // Have we already seen this one?
      if ( set.find( input_line ) != set.end() ) {
        std::ostringstream what;
        what << "line " << line
             << "contains a duplicate explanation ("
             << input_line << ")";
        throw rld::error( what, "Explanations::load" );
      }

      // Add the starting line and file
      e.startingPoint = input_line;
      e.found = false;

      // Get the classification
      std::getline( explain, input_line );
      if ( explain.fail() ) {
        std::ostringstream what;
        what << "line " << line
             << "out of sync at the classification";
        throw rld::error( what, "Explanations::load" );
      }
      e.classification = input_line;
      line++;

      // Get the explanation
      while ( std::getline( explain, input_line ) ) {
        line++;

        const std::string delimiter = "+++";
        if ( input_line.compare( delimiter ) == 0 ) {
          break;
        }
        // XXX only taking last line.  Needs to be a vector
        e.explanation.push_back( input_line );
      }

      if ( explain.fail() ) {
        std::ostringstream what;
        what << "line " << line
              << "out of sync at the explanation";
        throw rld::error( what, "Explanations::load" );
      }

      // Add this to the set of Explanations
      set[ e.startingPoint ] = e;
    }

  }

  const Explanation *Explanations::lookupExplanation(
    const std::string& start
  )
  {
    if ( set.find( start ) == set.end() ) {
      #if 0
        std::cerr << "Warning: Unable to find explanation for "
                  << start << std::endl;
      #endif
      return NULL;
    }
    set[ start ].found = true;
    return &set[ start ];
  }

  void Explanations::writeNotFound( const std::string& fileName )
  {
    std::ofstream notFoundFile;
    bool  notFoundOccurred = false;

    if ( fileName.empty() ) {
      return;
    }

    notFoundFile.open( fileName );
    if ( !notFoundFile ) {
      std::ostringstream what;
      what << "Unable to open " << fileName
           << " out of sync at the explanation";
      throw rld::error( what, "Explanations::writeNotFound" );
    }

    for (
      std::map<std::string, Explanation>::iterator itr = set.begin();
      itr != set.end();
      itr++
    ) {
      Explanation e = (*itr).second;
      std::string key = (*itr).first;

      if ( !e.found ) {
        notFoundOccurred = true;
        notFoundFile << e.startingPoint << std::endl;
      }
    }

    if ( !notFoundOccurred ) {
      if ( !unlink( fileName.c_str() ) ) {
        std::cerr << "Warning: Unable to unlink " << fileName
                  << std::endl
                  << std::endl;
      }
    }
  }

}
