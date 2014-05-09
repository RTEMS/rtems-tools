/*! @file rtemscov_header.h
 *  @brief rtemscov_header Implementation
 *
 *  This file contains the implementation of the functions supporting
 *  RTEMS Common Coverage Map file format.
 *
 */

#ifndef __RTEMS_Coverage_Header_h
#define __RTEMS_Coverage_Header_h

#define MAX_DESC_STR 32

/*!
 *
 *   This structure contains XXX
 */

typedef struct prof_header_s{
  /** the version of header file */
  int ver;
  /** The length of header */
  int header_length;
  /** starting address */
  int start;
  /** ending address */
  int end;
  /** The description info for profiling file */
  char desc[MAX_DESC_STR];
} rtems_coverage_map_header_t;

#endif
