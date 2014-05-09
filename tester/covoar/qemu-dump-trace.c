#include <stdint.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

#include "qemu-traces.h"


int dump_file(
  const char *name
)
{
  FILE *trace;
  struct trace_header header;
  struct trace_entry  entry;
  size_t              bytes;
  int                 instructions;

  trace = fopen( name, "r" );
  if ( !trace ) {
    perror( "unable to open trace file" );
    return -1;
  }

  bytes = fread( &header, sizeof(struct trace_header), 1, trace );
  if ( bytes != 1 ) {
    fprintf( stderr, "error reading header of %s (%s)\n",
             name, strerror(errno) );
    return -1;
  }
  printf( "magic = %s\n", header.magic );
  printf( "version = %d\n", header.version );
  printf( "kind = %d\n", header.kind );
  printf( "sizeof_target_pc = %d\n", header.sizeof_target_pc );
  printf( "big_endian = %d\n", header.big_endian );
  printf( "machine = %02x:%02x\n", header.machine[0], header.machine[1] );

  instructions = 0;
  while (1) {
    bytes = fread( &entry, sizeof(struct trace_entry), 1, trace );
    if ( bytes != 1 )
      break;
    instructions++;
    printf( "0x%08x %d 0x%2x\n", entry.pc, entry.size, entry.op );
  }

  
  fclose( trace );
  printf( "instructions = %d\n", instructions );
 
  return 0;

}

int main(
  int argc,
  char **argv
)
{
  int i;

  for (i=1 ; i<argc ; i++) {
    if ( dump_file( argv[i] ) )
      return -1;
  }
  return 0;
}
