/*-
 * Copyright (c) 2011 Chris Johns <chrisj@rtems.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Basic mmap/munmap set of functions that allows software that needs to use
 * these functions to work without changing. Currently only the basic read
 * has been tested.
 *
 * The basic was taken from the implementation in Python 3.x by Sam Rushing
 * <rushing@nightmare.com>.
 */

#ifndef __WIN32__
#error "Wrong OS; only for WIN32"
#endif

#include <errno.h>
#include <stdint.h>
#include <windows.h>

/*
 * Bring in the local mman.h header to make sure the interface is in sync.
 */
#include <sys/mman.h>

/*
 * The data for each map. Maintained as a list. If performance is important maybe
 * some other container can be used.
 */
typedef struct mmap_data_s
{
  struct mmap_data_s* next;
  void*               data;
  HANDLE              file_handle;
  HANDLE              map_handle;
  size_t              size;
  off_t               offset;
} mmap_data;

/*
 * Head of the map list.
 */
static mmap_data* map_head;

void*
mmap(void* addr, size_t len, int prot, int flags, int fd, off_t offset)
{
  mmap_data* map = NULL;
  DWORD      flProtect;
  DWORD      dwDesiredAccess;
  HANDLE     fh = 0;
  DWORD      dwErr = 0;
  DWORD      size_lo;
  DWORD      size_hi;
  DWORD      off_lo;
  DWORD      off_hi;
  uint64_t   size = 0;

  if ((fd == 0) || (fd == -1))
    return MAP_FAILED;

  /*
   * Not implemented. Patches welcome.
   */
  if (prot & PROT_EXEC)
    return MAP_FAILED;
  
  /*
   * Map the protection.
   */
  if ((prot & PROT_READ) == PROT_READ)
  {
    flProtect = PAGE_READONLY;
    dwDesiredAccess = FILE_MAP_READ;
  }
  if ((prot & PROT_WRITE) == PROT_WRITE)
  {
    flProtect = PAGE_WRITECOPY;
    dwDesiredAccess = FILE_MAP_WRITE;
  }
  if ((prot & (PROT_READ | PROT_WRITE)) == (PROT_READ | PROT_WRITE))
  {
    flProtect = PAGE_READWRITE;
    dwDesiredAccess = FILE_MAP_WRITE;
  }
  
  fh = (HANDLE) _get_osfhandle (fd);
  if (fh == (HANDLE) -1)
    return MAP_FAILED;

  /*
   * Win9x appears to need us seeked to zero.
   */
  lseek (fd, 0, SEEK_SET);

  /*
   * Allocate the space to handle the mapping.
   */
  map = malloc (sizeof (mmap_data));
  if (!map)
    return MAP_FAILED;

  map->next = NULL;
  map->data = NULL;
  map->file_handle = fh;
  map->map_handle = NULL;
  map->offset = offset;

  if (len == 0)
  {
    DWORD low;
    DWORD high;
    
    low = GetFileSize (fh, &high);
    
    /*
     * Low might just happen to have the value INVALID_FILE_SIZE; so we need to
     *  check the last error also.
    */
    if ((low == INVALID_FILE_SIZE) && (dwErr = GetLastError()) != NO_ERROR)
    {
      free (map);
      return MAP_FAILED;
    }

    size = (((uint64_t) high) << 32) + low;
    
    if (offset >= size)
    {
      free (map);
      return MAP_FAILED;
    }

    if (offset - size > (size_t) -1LL)
      /* Map area too large to fit in memory */
      map->size = (size_t) -1;
    else
      map->size = (size_t) (size - offset);
  }
  else
  {
    map->size = len;
    size = offset + len;
  }

  size_hi = (DWORD)(size >> 32);
  size_lo = (DWORD)(size & 0xFFFFFFFF);
  off_hi = (DWORD)(0);
  off_lo = (DWORD)(offset & 0xFFFFFFFF);
  
  /*
   * For files, it would be sufficient to pass 0 as size. For anonymous maps,
   * we have to pass the size explicitly.
   */
  map->map_handle = CreateFileMapping (map->file_handle,
                                       NULL,
                                       flProtect,
                                       size_hi,
                                       size_lo,
                                       NULL);
  if (!map->map_handle)
  {
    free (map);
    return MAP_FAILED;
  }
  
  map->data = (char *) MapViewOfFileEx (map->map_handle,
                                        dwDesiredAccess,
                                        off_hi,
                                        off_lo,
                                        map->size,
                                        addr);
  if (!map->data)
  {
    CloseHandle (map->map_handle);
    free (map);
    return MAP_FAILED;
  }

  /*
   * Add to the list.
   */
  map->next = map_head;
  map_head = map;
  
  return map->data;
}

int
munmap (void* addr, size_t len)
{
  mmap_data*  map = map_head;
  mmap_data** prev_next = &map_head;

  /*
   * Find the map and remove from the list.
   */
  while (map)
  {
    if (map->data == addr)
    {
      *prev_next = map->next;
      break;
    }
    prev_next = &map->next;
    map = map->next;
  }

  if (!map)
  {
    errno = EINVAL;
    return -1;
  }

  if (map->data != NULL)
    UnmapViewOfFile (map->data);
  if (map->map_handle != NULL)
    CloseHandle (map->map_handle);

  free (map);

  errno = 0;
  return 0;
}
