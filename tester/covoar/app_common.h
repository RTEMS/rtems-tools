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

extern Coverage::DesiredSymbols*    SymbolsToAnalyze;
extern bool                         BranchInfoAvailable;
extern Target::TargetBase*          TargetInfo;

#define MAX_LINE_LENGTH             512
extern char                         inputBuffer[MAX_LINE_LENGTH];
extern char                         inputBuffer2[MAX_LINE_LENGTH];


bool FileIsNewer( const char *f1, const char *f2 );
bool FileIsReadable( const char *f1 );
bool ReadUntilFound( FILE *file, const char *line );

#endif
