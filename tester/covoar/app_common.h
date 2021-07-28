#ifndef __APP_COMMON_h
#define __APP_COMMON_h

/*
 * This file needs to be removed and these globals removed from the
 * global scope. For example SymbolsToAnalyze is never destructed.
 */

#include <list>

#include "DesiredSymbols.h"
#include "Explanations.h"
#include "TargetBase.h"

extern Target::TargetBase*          TargetInfo;


bool FileIsNewer( const char *f1, const char *f2 );
bool FileIsReadable( const char *f1 );
bool ReadUntilFound( FILE *file, const char *line );

#endif
