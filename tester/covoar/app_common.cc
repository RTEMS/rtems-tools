/*
 * RTEMS Tools Project (http://www.rtems.org/)
 * Copyright 2014 OAR Corporation
 * All rights reserved.
 *
 * This file is part of the RTEMS Tools package in 'rtems-tools'.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "covoar-config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "app_common.h"
#include "DesiredSymbols.h"
#include "Explanations.h"

#if HAVE_STAT64
#define STAT stat64
#else
#define STAT stat
#endif

#if HAVE_OPEN64
#define OPEN fopen64
#else
#define OPEN fopen
#endif

/*
 *  Global variables for the program
 */
Coverage::DesiredSymbols*   SymbolsToAnalyze    = NULL;
bool                        Verbose             = false;
const char*                 outputDirectory     = ".";
bool                        BranchInfoAvailable = false;
Target::TargetBase*         TargetInfo          = NULL;
const char*                 dynamicLibrary      = NULL;
const char*                 projectName         = NULL;
char                        inputBuffer[MAX_LINE_LENGTH];
char                        inputBuffer2[MAX_LINE_LENGTH];


bool FileIsNewer(
  const char *f1,
  const char *f2
)
{
  struct STAT buf1, buf2;

   if (STAT( f2, &buf2 ) == -1)
    return true;

  if (STAT( f1, &buf1 ) == -1)
    exit (1);

  if (buf1.st_mtime > buf2.st_mtime)
    return true;

  return false;
}

bool FileIsReadable( const char *f1 )
{
  struct STAT buf1;

  if (STAT( f1, &buf1 ) == -1)
    return false;

  if (buf1.st_size == 0)
    return false;

  // XXX check permission ??
  return true;
}

bool ReadUntilFound( FILE *file, const char *line )
{
  char discardBuff[100];
  size_t  len = strlen( line );

  do {
    if ( !fgets( discardBuff, 99, file ) )
      return false;

    if ( strncmp( discardBuff, line, len ) == 0 )
      return true;
  } while (1);
}

