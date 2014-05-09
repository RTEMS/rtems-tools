/*! @file skyeye_header.h
 *  @brief skyeye_header Implementation
 *
 *  This file contains the definition of the format
 *  for coverage files written by the Skyeye simulator.
 */

#ifndef __Skyeye_Header_h
#define __Skyeye_Header_h

#define MAX_DESC_STR 32

typedef struct prof_header_s {
  int ver;                 /** the version of header file */
  int header_length;       /** The length of header */
  int prof_start;
  int prof_end;
  char desc[MAX_DESC_STR]; /** The description info for profiling file */
} prof_header_t;

#endif
