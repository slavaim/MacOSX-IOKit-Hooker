//
//  HookExample.cpp
//  IOKitHooker
//
//  Created by slava on 4/12/2016.
//  Copyright (c) 2016 Slava-Imameev. All rights reserved.
//

#include "HookExample.h"
#include "DldIOKitHookEngine.h"

//--------------------------------------------------------------------

bool
HookExample()
{
    //
    // initialize the hooking subsystem
    //
    gHookEngine = DldIOKitHookEngine::withNoHooks();
    assert( NULL != gHookEngine );
    if( NULL == gHookEngine ) {
        
        DBG_PRINT_ERROR( ( "DldIOKitHookEngine::withNoHooks() failed\n" ) );
        return false;
    }
    
    //
    // start IOUserClient class derived objects hooking, the hooks might be called before this function returns!
    //
    if( !gHookEngine->startHookingWithPredefinedClasses() ){
        
        DBG_PRINT_ERROR( ( "gHookEngine->startHookingWithPredefinedClasses() failed\n" ) );
        return false;
    }
    
    return true;
}

//--------------------------------------------------------------------
