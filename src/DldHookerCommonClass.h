/*
 * Copyright (c) 2009 Slava Imameev. All rights reserved.
 */

#ifndef _DLDHOOKERCOMMONCLASS_H
#define _DLDHOOKERCOMMONCLASS_H

#include <IOKit/IOService.h>
#include <IOKit/IOUserClient.h>
#include <IOKit/IODataQueue.h>
#include <IOKit/assert.h>
#include <sys/types.h>
#include <sys/kauth.h>
#include <kern/locks.h>
#include "DldCommon.h"
#include "DldCommonHashTable.h"


//--------------------------------------------------------------------

class DldHookerBaseInterface{
    
protected:
    
    virtual bool init() = 0;
    virtual void free() = 0;
    
public:
    virtual const char* fGetClassName() = 0;
};

//--------------------------------------------------------------------

typedef enum _DldInheritanceDepth{
    DldInheritanceDepth_0 = 0,// the leaf class, i.e. not a parent or ancestor
    DldInheritanceDepth_1,
    DldInheritanceDepth_2,
    DldInheritanceDepth_3,
    DldInheritanceDepth_4,
    DldInheritanceDepth_5,
    DldInheritanceDepth_6,
    DldInheritanceDepth_7,
    DldInheritanceDepth_8,
    DldInheritanceDepth_9,
    DldInheritanceDepth_10,
    DldInheritanceDepth_Maximum
} DldInheritanceDepth;

//--------------------------------------------------------------------

typedef void (*DldVtableFunctionPtr)(void);

typedef struct _DldHookedFunctionInfo{
    
    //
    // an index for a function in the hooked class vtable,
    // (-1) is a sign of a trailing descriptor for an array
    //
    unsigned int            VtableIndex;
    
    //
    // an original function address from the hooked class vtable
    //
    DldVtableFunctionPtr    OriginalFunction;
    
    //
    // an address of hooking function which replaces the original one
    //
    DldVtableFunctionPtr    HookingFunction;
    
} DldHookedFunctionInfo;

//--------------------------------------------------------------------

typedef enum _DldHookType{
    
    DldHookTypeUnknown = 0x0,
    
    //
    // the poiter to a vtable for an object is replaced thus
    // the hook affects only the object,
    // the key is of OSObject* type
    //
    DldHookTypeObject = 0x1,
    
    //
    // the function adresses are replaced in the original vtable thus
    // affecting all objects of the same type,
    // the keys are of the DldHookTypeVtableObjKey & DldHookTypeVtableKey types
    //
    DldHookTypeVtable = 0x2,
    
    //
    // a terminating value, not an actual type
    //
    DldHookTypeMaximum
    
} DldHookType;

//
// a key for the object hooked by a direct vtable hook ( i.e. the original vtable has been tampered with ),
// the metaClass defines the class for which the vtable hook has been done
//
typedef struct _DldHookTypeVtableObjKey{
    const OSObject* Object;
    const OSMetaClass* metaClass;
    DldInheritanceDepth InheritanceDepth;
} DldHookTypeVtableObjKey;

//
// a key for the a vtable hooked by a direct vtable hook ( i.e. the original vtable has been tampered with ),
// the metaClass defines the class for which the vtable hook has been done
//
typedef struct _DldHookTypeVtableKey{
    //
    // there are two keys for Vtabe entry - one with original Vtable and the second with NULL
    //
    OSMetaClassBase::_ptf_t* Vtable;
    const OSMetaClass* metaClass;
    DldInheritanceDepth InheritanceDepth;
} DldHookTypeVtableKey;

//
// a key used only in the debug buils, saves the reverse transformation from the address
// of the hooked vtable's entry to the object which was used for hooking
//
typedef struct _DldDbgVtableHookToObject{
    struct _DldDbgVtableHookToObject* copy;
    const OSObject*           Object;
    const OSMetaClass*        MetaClass;
    OSMetaClassBase::_ptf_t*  Vtable;
    OSMetaClassBase::_ptf_t*  VtableEntry;// also a key
    DldHookedFunctionInfo*    HookInfo;
} DldDbgVtableHookToObject;

//
// this is more a structure than a class, the OSObject as a base class is used for a purpose of reference counting
//
class DldHookedObjectEntry: public OSObject{
    
    OSDeclareDefaultStructors( DldHookedObjectEntry )
    
public:
    
    static DldHookedObjectEntry* allocateNew();
    
protected:
    
    virtual void free();
    
public:
    
    union{
        
        //
        // DldHookEntryTypeObject
        //
        OSObject* Object;
        
        //
        // DldHookEntryTypeVtableObj
        //
        DldHookTypeVtableObjKey    VtableHookObj;
        
        //
        // DldHookEntryTypeVtable for non null entry, for null entry
        // a key can be constructed from this one by setting the Vtable value to NULL
        //
        DldHookTypeVtableKey       VtableHookVtable;
        
    } Key;
    
    typedef enum _DldEntryType{
        DldHookEntryTypeUnknown = 0x0,
        DldHookEntryTypeObject,   // OSObject* key
        DldHookEntryTypeVtableObj,// DldHookTypeVtableObjKey key
        DldHookEntryTypeVtable    // DldHookTypeVtableKey key
    } DldEntryType;
    
    DldEntryType   Type;
    
    union{
        
        struct CommonHeader{
            
            //
            // the value of the HookedVtableFunctionsInfo member is as follows
            //
            
            //
            // for an entry of the DldHookEntryTypeObject type:
            //    a cahed value for ClassHookerObject->HookedFunctonsInfo
            //    must NOT be freed when the object of the DldHookEntryTypeObject
            //    type is freed
            //
            
            //
            // for an entry of the DldHookEntryTypeVtableObj type:
            //    a cahed value for VtableEntry->HookedVtableFunctionsInfo,
            //    must NOT be freed when the entry of the DldHookEntryTypeVtableObj
            //    type is freed as will be freed when the related entry of the
            //    DldHookEntryTypeVtable type is freed
            //
            
            //
            // for an entry of the DldHookEntryTypeVtable type:
            //    a pointer to an array, used only if the entry describes
            //    a directly hooked vtable which can be found by DldHookTypeVtableKey,
            //    the last entry in the array contains (-1) as an index,
            //    the entry type is DldHookEntryTypeVtable, the array must be alocated
            //    by calling IOMalloc and will be freed when the entry is finalized
            //    by calling IOFree
            //
            
            DldHookedFunctionInfo*        HookedVtableFunctionsInfo;
            
        } Common;
        
        //
        // DldHookEntryTypeObject
        //
        struct{
            
            //
            // always a first member!
            //
            struct CommonHeader           Common;
            
            //
            // an original Vtable address
            //
            OSMetaClassBase::_ptf_t*      OriginalVtable;
            
        } TypeObject;
        

        //
        // DldHookEntryTypeVtableObj
        //
        struct{
            
            //
            // always a first member!
            //
            struct CommonHeader           Common;
            
            //
            // a pointer to a referenced entry for a hooked vtable which
            // can be found by DldHookTypeVtableKey, used if the entry
            // describes the object with the directly hooked vtable
            // i.e. has the DldHookEntryTypeVatbleObj type
            //
            DldHookedObjectEntry*         VtableEntry;
            
            //
            // an original Vtable address
            //
            OSMetaClassBase::_ptf_t*      OriginalVtable;
            
        } TypeVatbleObj;
        

        //
        // DldHookEntryTypeVtable
        //
        struct{
            
            //
            // always a first member!
            //
            struct CommonHeader           Common;

            unsigned int                  HookedVtableFunctionsInfoEntriesNumber;
            
            //
            // when the reference count drops to zero the entry is removed from
            // the hash and the Vtable is unhooked
            //
            unsigned int                  ReferenceCount;

            
        } TypeVtable;
        
    } Parameters;
    
    //
    // an inheritance depth
    //
    DldInheritanceDepth    InheritanceDepth;
    
    //
    // a hooking class object, not referenced
    //
    DldHookerBaseInterface* ClassHookerObject;
};

//--------------------------------------------------------------------

class DldHookedObjectsHashTable
{
    
private:
    
    ght_hash_table_t* HashTable;
    IORWLock*         RWLock;
    
#if defined(DBG)
    thread_t ExclusiveThread;
#endif//DBG
    
    //
    // returns an allocated hash table object
    //
    static DldHookedObjectsHashTable* withSize( int size, bool non_block );
    
    //
    // free must be called before the hash table object is deleted
    //
    void free();
    
    //
    // as usual for IOKit the desctructor and constructor do nothing
    // as it is impossible to return an error from the constructor
    // in the kernel mode
    //
    DldHookedObjectsHashTable()
    {
        this->HashTable = NULL;
        
#if defined(DBG)
        this->ExclusiveThread = NULL;
#endif//DBG
    }
    
    //
    // the destructor checks that the free() has been called
    //
    ~DldHookedObjectsHashTable(){ assert( !this->HashTable && !this->RWLock ); };
    
public:
    
    static bool CreateStaticTableWithSize( int size, bool non_block );
    static void DeleteStaticTable();
    
    //
    // the objEntry is referenced by AddObject, so the caller should release the object
    // if the true value is returned
    //
    bool   AddObject( __in OSObject* obj, __in DldHookedObjectEntry* objEntry, __in bool errorIfPresent = true );
    bool   AddObject( __in DldHookTypeVtableKey* vtableHookVtable, __in DldHookedObjectEntry* objEntry, __in bool errorIfPresent = true );
    bool   AddObject( __in DldHookTypeVtableObjKey* vtableHookObj, __in DldHookedObjectEntry* objEntry, __in bool errorIfPresent = true );
    
    //
    // removes the entry from the hash and returns the removed entry, NULL if there
    // is no entry for an object or vtable, the returned entry is referenced!
    //
    DldHookedObjectEntry*   RemoveObject( __in OSObject* obj );
    DldHookedObjectEntry*   RemoveObject( __in DldHookTypeVtableKey* vtableHookVtable );
    DldHookedObjectEntry*   RemoveObject( __in DldHookTypeVtableObjKey* vtableHookObj );
    
    //
    // the returned object is referenced if the reference parameter is true! the caller must release the object!
    //
    DldHookedObjectEntry*   RetrieveObjectEntry( __in OSObject* obj, __in bool reference = true );
    DldHookedObjectEntry*   RetrieveObjectEntry( __in DldHookTypeVtableKey* vtableHookVtable, __in bool reference = true );
    DldHookedObjectEntry*   RetrieveObjectEntry( __in DldHookTypeVtableObjKey* vtableHookObj, __in bool reference = true );
    
#if defined( DBG )
    //
    // used only for the debug, leaks memory in the release
    //
    bool AddObject( __in OSMetaClassBase::_ptf_t* key, __in DldDbgVtableHookToObject* DbgEntry, __in bool errorIfPresent = true );
    DldDbgVtableHookToObject* RetrieveObjectEntry( __in OSMetaClassBase::_ptf_t*    key );
#endif//DBG
    
    void
    LockShared()
    {   assert( this->RWLock );
        assert( preemption_enabled() );
        
        IORWLockRead( this->RWLock );
    };
    
    
    void
    UnLockShared()
    {   assert( this->RWLock );
        assert( preemption_enabled() );
        
        IORWLockUnlock( this->RWLock );
    };
    
    
    void
    LockExclusive()
    {
        assert( this->RWLock );
        assert( preemption_enabled() );
        
#if defined(DBG)
        assert( current_thread() != this->ExclusiveThread );
#endif//DBG
        
        IORWLockWrite( this->RWLock );
        
#if defined(DBG)
        assert( NULL == this->ExclusiveThread );
        this->ExclusiveThread = current_thread();
#endif//DBG
        
    };
    
    
    void
    UnLockExclusive()
    {
        assert( this->RWLock );
        assert( preemption_enabled() );
        
#if defined(DBG)
        assert( current_thread() == this->ExclusiveThread );
        this->ExclusiveThread = NULL;
#endif//DBG
        
        IORWLockUnlock( this->RWLock );
    };
    
    
    static DldHookedObjectsHashTable* sHashTable;
};

//--------------------------------------------------------------------

//
// a definition extracted from OSMetaClass.h,
// a definition for a single inherited class in the Apple ABI ( i.e. the same as in gcc family )
//
typedef union _DldSingleInheritingClassObjectPtr{
    const OSObject *fObj;// a pointer to the object
    OSMetaClassBase::_ptf_t **vtablep;// a pointer to the object's vtable pointer
} DldSingleInheritingClassObjectPtr;

typedef union _DldSingleInheritingHookerClassObjectPtr{
    const DldHookerBaseInterface *fObj;// a pointer to the object
    OSMetaClassBase::_ptf_t **vtablep;// a pointer to the object's vtable pointer
} DldSingleInheritingHookerClassObjectPtr;

//--------------------------------------------------------------------

#ifndef APPLE_KEXT_LEGACY_ABI// 10.5 SDK lacks this definition

    #if defined(__LP64__)
        /*! @parseOnly */
        #define APPLE_KEXT_LEGACY_ABI  0
    #elif defined(__arm__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 2))
        #define APPLE_KEXT_LEGACY_ABI  0
    #else
        #define APPLE_KEXT_LEGACY_ABI  1
    #endif

#endif//APPLE_KEXT_LEGACY_ABI


#if APPLE_KEXT_LEGACY_ABI

//
// a definition extracted from OSMetaClass.h,
// arcane evil code interprets a C++ pointer to function as specified in the
// -fapple-kext ABI, i.e. the gcc-2.95 generated code.  IT DOES NOT ALLOW
// the conversion of functions that are from MULTIPLY inherited classes.
//
typedef union _DldClassMemberFunction{
    void (OSMetaClassBase::*fIn)(void);
    struct { 	// Pointer to member function 2.95
        unsigned short fToff;
        short  fVInd;
        union {
            OSMetaClassBase::_ptf_t fPFN;
            short  fVOff;
        } u;
    } fptmf2;
} DldClassMemberFunction;

//
// a function extracted from OSMetaClass.h,
// returns a vtable index starting from 0x1 for a function or (-1) if the function is not virtual,
// this function was crafted from the _ptmf2ptf function, also se the OSMemberFunctionCast
// macro for reference
//
// @param func The pointer to member function itself, something like &Base::func.
//

//
// Arcane evil code interprets a C++ pointer to function as specified in the
// -fapple-kext ABI, i.e. the gcc-2.95 generated code.  IT DOES NOT ALLOW
// the conversion of functions that are from MULTIPLY inherited classes.
//

static 
//#if !defined(DBG)
inline
//#endif//!DBG
unsigned int
DldConvertFunctionToVtableIndex(
    __in void (OSMetaClassBase::*func)(void)
    )
{
    DldClassMemberFunction map;
    
    map.fIn = func;
    if (map.fptmf2.fToff){
        
        DBG_PRINT_ERROR(("A non virtual function is passed to DldConvertFunctionToVtableIndex as a parameter"));
        assert( !"Multiple inheritance is not supported" );
#if defined( DBG )
        panic( "Multiple inheritance is not supported" );
#endif//DBG
        return (unsigned int)(-1);
        
    } else if (map.fptmf2.fVInd < 0) {
        
        //
        // Not a virtual, i.e. plain member func
        //
        DBG_PRINT_ERROR(("A non virtual function is passed to DldConvertFunctionToVtableIndex as a parameter"));
        assert( !"A non virtual function is passed to DldConvertFunctionToVtableIndex as a parameter" );
        return (unsigned int)(-1);
        
    } else {
        
        //
        // a virtual member function so return the index,
        // the base is 0x1
        //
        return map.fptmf2.fVInd;
    }
    
    panic( "We should not be here!" );
    return (unsigned int)(-1);
}

#else /* !APPLE_KEXT_LEGACY_ABI */

#ifdef __arm__
#error "we doesn't support ARM here"
#else /* __arm__ */

//
// a function extracted from OSMetaClass.h,
// returns a vtable index starting from 0x1 for a function or (-1) if the function is not virtual,
// this function was crafted from the _ptmf2ptf function, also se the OSMemberFunctionCast
// macro for reference
//
// @param func The pointer to member function itself, something like &Base::func.
//

//
// Slightly less arcane and slightly less evil code to do
// the same for kexts compiled with the standard Itanium C++
// ABI
//

static 
//#if !defined(DBG)
inline
//#endif//!DBG
unsigned int
DldConvertFunctionToVtableIndex(
    __in void (OSMetaClassBase::*func)(void)
    )
{
    union {
        void (OSMetaClassBase::*fIn)(void);
        uintptr_t fVTOffset;
        OSMetaClassBase::_ptf_t fPFN;
    } map;
    
    map.fIn = func;
    
    if (map.fVTOffset & 1) {
        //
        // virtual
        //
        
        //
        // Virtual member function so dereference vtable,
        // the base is 0x1
        //
        return (map.fVTOffset - 1)/sizeof( OSMetaClassBase::_ptf_t )+0x1;
    } else {
        
        //
        // Not virtual, i.e. plain member func
        //
        DBG_PRINT_ERROR(("Not virtual, i.e. plain member func has been passed to DldConvertFunctionToVtableIndex"));
        assert( !"Not virtual, i.e. plain member func has been passed to DldConvertFunctionToVtableIndex" );
#if defined( DBG )
        panic( "Not virtual, i.e. plain member func has been passed to DldConvertFunctionToVtableIndex" );
#endif//DBG
        return (unsigned int)(-1);
    }
    
    panic( "We should not be here!" );
    return (unsigned int)(-1);
}

#endif /* __arm__ */

#endif /* !APPLE_KEXT_LEGACY_ABI */


//--------------------------------------------------------------------

#define DldDeclareGetClassNameFunction()\
virtual const char* fGetClassName()\
{\
    char* name = (char*)__PRETTY_FUNCTION__; \
    int i;\
\
    for( i = 0x0; ':' != name[i] && '\0' != name[i]; ++i ){;}\
\
    if( '\0' != name[i] )\
        name[i] = '\0';\
    return (const char*)name;\
};

//--------------------------------------------------------------------

//
// the class is just a container for data and functions common for
// all hookers to avoid code duplication accross all hokers, it
// was introduced for convinience and doesn't take any reference,
// a class object is alive until containing it hooking object is alive,
// a class should not be instantiated on its own as a standalone object
//
class DldHookerCommonClass
{
    
private:
    
    IOReturn HookVtableIntWoLock( __inout OSObject* object );
    IOReturn HookObjectIntWoLock( __inout OSObject* object );
    IOReturn UnHookObjectIntWoLock( __inout OSObject* object );
    
    //
    // a type of the hook performed by the class
    //
    DldHookType                  HookType;
    
    //
    // an inheritance depth of the hooked class
    //
    DldInheritanceDepth          InheritanceDepth;
    
    //
    // a meta class and a name for the hooked class, the fields initialization is delayed
    // till the first object is being hooked
    //
    const OSMetaClass*            MetaClass;
    const OSSymbol*               ClassName; // retained
    
    //
    // a number of hooked objects, valid only for DldHookTypeObject,
    //
    UInt32                        HookedObjectsCounter;
    
    //
    // the object is not referenced( can't be as not derived from OSObject )
    //
    DldHookerBaseInterface*       ClassHookerObject;
    
    OSMetaClassBase::_ptf_t*      OriginalVtable;
    OSMetaClassBase::_ptf_t*      HookClassVtable;
    OSMetaClassBase::_ptf_t*      NewVtable;// NULL if the direct vtable hook
    
    //
    // a buffer allocated for a new vtable
    //
    void*                         Buffer;
    vm_size_t                     BufferSize;                         
    
    //
    // a hooked class vtable's size, defines only the size of the table for 
    // a class which was used to define this hooking class, the actual table size
    // is defined by ( HookedVtableSize + VirtualsAdded )
    //
    unsigned int                  HookedVtableSize;
    
    //
    // defines the number of virtual functions added by children if this hooking class is defined through
    // a parent of the hooked class, usually this happens when the leaf class is
    // not exported for third party developers ( as in the case of AppleUSBEHCI )
    //
    // the VirtualsAddedSize member is initialized when the first object is being hooked as a 
    // real object is required to investigate its vtable
    //
    unsigned int                   VirtualsAddedSize;
    
    //
    // a pointer to array of vtable's functions info,
    // provided by a class containing an instance of this one as a member
    //
    DldHookedFunctionInfo*        HookedFunctonsInfo;
    
    //
    // a number of entries ( including terminating ) in the HookedFunctonsInfo array
    //
    unsigned int                  HookedFunctonsInfoEntriesNumber;
    
public:
    
    DldHookerCommonClass();   
    ~DldHookerCommonClass();
    
    //
    // saves an object pointer, the object is not referenced
    //
    void SetClassHookerObject( __in DldHookerBaseInterface*  ClassHookerObject );
    
    void SetHookedVtableSize( __in unsigned int HookedVtableSize );
    
    void SetInheritanceDepth( __in DldInheritanceDepth   Depth );
    
    //void SetMetaClass( __in const OSMetaClass*    MetaClass );
    
    //
    // NumberOfEntries includes a terminating entry
    //
    void SetHookedVtableFunctionsInfo( __in DldHookedFunctionInfo* HookedFunctonsInfo, __in unsigned int NumberOfEntries );
    
    //
    // returns unreferenced object
    //
    DldHookerBaseInterface* GetClassHookerObject(){ return this->ClassHookerObject; };
    
    DldHookType GetHookType(){ return this->HookType; };
    
    DldInheritanceDepth GetInheritanceDepth(){ return this->InheritanceDepth; };
    
    //
    // callbacks called by hooking functions ( the name defines the corresponding IOService hook )
    //
    bool start( __in IOService* serviceObject, __in IOService * provider );
    
    bool open( __in IOService* serviceObject, __in IOService * forClient, __in IOOptionBits options, __in void * arg );
    
    void free( __in IOService* serviceObject );
    
    bool terminate( __in IOService* serviceObject, __in IOOptionBits options );
    
    bool finalize( __in IOService* serviceObject, __in IOOptionBits options );
    
    bool attach( __in IOService* serviceObject, __in IOService * provider );
    
    bool attachToChild( __in IOService* serviceObject, IORegistryEntry * child,const IORegistryPlane * plane );
    
    void detach( __in IOService* serviceObject, __in IOService * provider );
    
    bool requestTerminate( __in IOService* serviceObject, __in IOService * provider, __in IOOptionBits options );
    
    bool willTerminate( __in IOService* serviceObject, __in IOService * provider, __in IOOptionBits options );
    
    bool didTerminate( __in IOService* serviceObject, __in IOService * provider, __in IOOptionBits options, __inout bool * defer );
    
    bool terminateClient( __in IOService* serviceObject, __in IOService * client, __in IOOptionBits options );
    
    void setPropertyTable( __in IOService* serviceObject, __in OSDictionary * dict );
    
    bool setProperty1( __in IOService* serviceObject, const OSSymbol * aKey, OSObject * anObject );
    bool setProperty2( __in IOService* serviceObject, const OSString * aKey, OSObject * anObject );
    bool setProperty3( __in IOService* serviceObject, const char * aKey, OSObject * anObject );
    bool setProperty4( __in IOService* serviceObject, const char * aKey, const char * aString );
    bool setProperty5( __in IOService* serviceObject, const char * aKey, bool aBoolean );
    bool setProperty6( __in IOService* serviceObject, const char * aKey, unsigned long long aValue, unsigned int aNumberOfBits );
    bool setProperty7( __in IOService* serviceObject, const char * aKey, void * bytes, unsigned int length );
     
    void removeProperty1( __in IOService* serviceObject, const OSSymbol * aKey);
    void removeProperty2( __in IOService* serviceObject, const OSString * aKey);
    void removeProperty3( __in IOService* serviceObject, const char * aKey);
    
    IOReturn newUserClient1( __in IOService* serviceObject, task_t owningTask, void * securityID,
                            UInt32 type,  OSDictionary * properties, IOUserClient ** handler );

    
    
    //
    // callbacks called as a reaction on some events happening with ah object
    //
    bool ObjectFirstPublishCallback( __in IOService* newService );
    
    bool ObjectTerminatedCallback( __in IOService* terminatedService );
    
    IOReturn HookObject( __inout OSObject* object, __in DldHookType type );
    
    IOReturn UnHookObject( __inout OSObject* object );
    
    OSMetaClassBase::_ptf_t GetOriginalFunction( __in OSObject* hookedObject, __in unsigned int indx );
    
    //
    // the VtableToHook and NewVtable vtables might be the same in case of a direct hook ( DldHookTypeVtable )
    //
    static void DldHookVtableFunctions( __in OSObject*                        object, // used only for the debug
                                        __inout DldHookedFunctionInfo*        HookedFunctonsInfo,
                                        __inout OSMetaClassBase::_ptf_t*      VtableToHook,
                                        __inout OSMetaClassBase::_ptf_t*      NewVtable );
    
    static void DldUnHookVtableFunctions( __inout DldHookedFunctionInfo*        HookedFunctonsInfo,
                                          __inout OSMetaClassBase::_ptf_t*      VtableToUnHook );
    
};

//--------------------------------------------------------------------

#endif//_DLDHOOKERCOMMONCLASS_H
