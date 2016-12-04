/* 
 * Copyright (c) 2010 Slava Imameev. All rights reserved.
 */

#include "DldUndocumentedQuirks.h"
#include <sys/lock.h>
#include <sys/proc.h>

//--------------------------------------------------------------------

//
// it is hardly that we will need more than CPUs in the system,
// but we need a reasonable prime value for a good dustribution
//
static IOSimpleLock*    gPreemptionLocks[ 19 ];

//--------------------------------------------------------------------

bool
DldAllocateInitPreemtionLocks()
{
    
    for( int i = 0x0; i < DLD_STATIC_ARRAY_SIZE( gPreemptionLocks ); ++i ){
        
        gPreemptionLocks[ i ] = IOSimpleLockAlloc();
        assert( gPreemptionLocks[ i ] );
        if( !gPreemptionLocks[ i ] )
            return false;
        
    }
    
    return true;
}

//--------------------------------------------------------------------

void
DldFreePreemtionLocks()
{
    for( int i = 0x0; i < DLD_STATIC_ARRAY_SIZE( gPreemptionLocks ); ++i ){
        
        if( gPreemptionLocks[ i ] )
            IOSimpleLockFree( gPreemptionLocks[ i ] );
        
    }
}

//--------------------------------------------------------------------

#define DldTaskToPreemptionLockIndx( _t_ ) (int)( ((vm_offset_t)(_t_)>>5)%DLD_STATIC_ARRAY_SIZE( gPreemptionLocks ) )

//
// MAC OS X kernel doesn't export disable_preemtion() and does not
// allow a spin lock allocaton on the stack! Marvelous, first time
// I saw such a reckless design! All of the above are allowed by
// Windows kernel as IRQL rising to DISPATCH_LEVEL and KSPIN_LOCK.
//

//
// returns a cookie for DldEnablePreemption
//
int
DldDisablePreemption()
{
    int indx;
    
    //
    // check for a recursion or already disabled preemption,
    // we support a limited recursive functionality - the
    // premption must not be enabled by calling enable_preemption()
    // while was disabled by this function
    //
    if( !preemption_enabled() )
        return (int)(-1);
    
    indx = DldTaskToPreemptionLockIndx( current_task() );
    assert( indx < DLD_STATIC_ARRAY_SIZE( gPreemptionLocks ) );
    
    //
    // acquiring a spin lock have a side effect of preemption disabling,
    // so try to find any free slot for this thread
    //
    while( !IOSimpleLockTryLock( gPreemptionLocks[ indx ] ) ){
        
        indx = (indx+1)%DLD_STATIC_ARRAY_SIZE( gPreemptionLocks );
        
    }// end while
    
    assert( !preemption_enabled() );
    
    return indx;
}

//--------------------------------------------------------------------

//
// accepts a value returned by DldDisablePreemption
//
void
DldEnablePreemption( __in int cookie )
{
    assert( !preemption_enabled() );
    
    //
    // check whether a call to DldDisablePreemption was a void one
    //
    if( (int)(-1) == cookie )
        return;
    
    assert( cookie < DLD_STATIC_ARRAY_SIZE( gPreemptionLocks ) );
    
    //
    // release the lock thus enabling preemption
    //
    IOSimpleLockUnlock( gPreemptionLocks[ cookie ] );
    
    assert( preemption_enabled() );
}

//--------------------------------------------------------------------

bool
DldInitUndocumentedQuirks()
{
    
    assert( preemption_enabled() );
    assert( current_task() == kernel_task );
    
    if( !DldAllocateInitPreemtionLocks() ){
        
        DBG_PRINT_ERROR(("DldAllocateInitPreemtionLocks() failed\n"));
        return false;
    }
    
    return true;
}

//--------------------------------------------------------------------

void
DldFreeUndocumentedQuirks()
{
    DldFreePreemtionLocks();
}

//--------------------------------------------------------------------
