/* 
 * Copyright (c) 2010 Slava Imameev. All rights reserved.
 */

#ifndef DLDUNDOCUMENTEDQUIRKS_H
#define DLDUNDOCUMENTEDQUIRKS_H

#include <sys/types.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/vm.h>
#include "DldCommon.h"

//--------------------------------------------------------------------

bool DldInitUndocumentedQuirks();

void DldFreeUndocumentedQuirks();

//--------------------------------------------------------------------

int DldDisablePreemption();

void DldEnablePreemption( __in int cookie );

//--------------------------------------------------------------------

#endif//DLDUNDOCUMENTEDQUIRKS_H