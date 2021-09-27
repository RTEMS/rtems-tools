#include "TraceList.h"
#include <iostream>
#include <iomanip>

namespace Trace {

    TraceList::TraceList()
    {
    }

    TraceList::~TraceList()
    {
    }

    void TraceList::add(
      uint32_t         lowAddressArg,
      uint32_t         highAddressArg,
      exitReason_t     why
    )
    {
      traceRange_t t;

      t.lowAddress = lowAddressArg;
      t.length     = highAddressArg - lowAddressArg;
      t.exitReason = why;

      set.push_back( t );
    }

    void TraceList::ShowTrace( traceRange_t *t)
    {
      std::cout << std::hex << "Start 0x" << t->lowAddress
                << ", length 0x" << std::setfill( '0' ) << std::setw( 3 )
                << t->length << " Reason " << t->exitReason << std::endl;
    }

    void  TraceList::ShowList()
    {
      ranges_t::iterator   ritr;
      traceRange_t         t;

      for ( ritr=set.begin(); ritr != set.end(); ritr++ ) {
        t = *ritr;
        ShowTrace( &t );
      }
    }

}
