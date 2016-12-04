# MacOSX-IOKit-Hooker

##License

The license model is a BSD Open Source License. This is a non-viral license, only asking that if you use it, you acknowledge the authors, in this case Slava Imameev.

##Features

This is a hooker for macOS (Mac OS X) IOKit class objects. This hooker is a part of a bigger project MacOSX-Kernel-Filter https://github.com/slavaim/MacOSX-Kernel-Filter. I extracted the hooker related code and removed unrelated dependencies to make it easy to incorporate the hooker for new projects. The repository contains an IOKit module project that is provided only for your convinience so you can check that files can be compiled as a standalone project in your build environment.

The src directory contains the hooker code. The example directory contains an example of hooker implementation for IOUserClient derived objects.

Apple I/O Kit is a set of classes to develop kernel modules for Mac OS X and iOS. Its analog in the Windows world is KMDF/UMDF framework. I/O Kit is built atop Mach and BSD subsystems like Windows KMDF is built atop WDM and kernel API.  
  
The official way to develop a kernel module filter for Mac OS X and iOS is to inherit filter C++ class from a C++ class it filters. This requires access to a class declaration which is not always possible as some classes are Apple private or not published by a third party developers. In most cases all these private classes are inherited from classes that are in the public domain. That means they extend an existing interface/functionality and a goal to filter device I/O can be achieved by filtering only the public interface. This is nearly always true because an I/O Kit class object that is attached to this private C++ class object is usually an Apple I/O Kit class that knows nothing about the third party extended interface or is supposed to be from a module developed by third party developers and it knows only about a public interface. In both cases the attached object issues requests to a public interface.  
  
Let's consider some imaginary private class IOPrivateInterface that inherits from some IOAppleDeviceClass which declaration is available and an attached I/O Kit object issues requests to IOAppleDeviceClass interface  
  
`class IOPrivateInterface: public IOAppleDeviceClass {   
};`  
  
You want to filter requests to a device managed by IOPrivateInterface, that means that you need to declare your filter like  
  
`class IOMyFilter: public IOPrivateInterface{  
};`  
  
this would never compile as you do not have access to IOPrivateInterface class. You can't declare you filter as  

`class IOMyFilter: public IOAppleDeviceClass {
};`  

as this will jettison all IOPrivateInterface code and the device will not function correctly.  

There might be another reason to avoid standard I/O Kit filtering by inheritance. A filtering class objects replaces an original class object in the I/O Kit device stack. That means a module with a filter should be available on device discovery and initialization. In nearly all cases this means that an instance of a filter class object will be created during system startup. This puts a great responsibility on a module developer as an error might render the system unbootable without an easy solution for a customer to fix the problem.  
  
To overcome these limitations I developed a hooking technique for I/O kit classes. I/O Kit uses virtual C++ functions so a class can be extended but its clients still be able to use a base class declaration. That means that all functions that used for I/O are placed in the array of function pointers known as vtable.  
  
The hooking technique supports two types of hooking.
  - replacing vtable array pointer in class object  
  - replacing selected functions in vtable array without changing vtable pointer  
  
The former method allows to filter access to a particular object of a class but requires knowing a vtable size. The latter method allows to filter request without knowing vtable size but as vtable is shared by all objects of a class a filter will see requests to all objects of a particular class. To get a vtable size you need a class declaration or get the size by reverse engineering.  
  
The hooker code can be found in DldHookerCommonClass.cpp .   
  
The driver uses C++ templates to avoid code duplication. Though Apple documentation declares that C++ templates are not supported by I/O Kit it is true only if a template is used to declare I/O kit object. You can compile I/O kit module with C++ template classes if they are not I/O Kit classes but template parameters can be I/O Kit classes. As you probably know after instantiation a template is just an ordinary C++ class. Template classes support is not required from run time environment. You can't declare I/O Kit class as a template just because a way Apple declares them by using C style preprocessor definitions. 

Below is a call stack when a hooked I/O Kit object virtual function is called by IOStorage::open
 
 ```
DldIOService::open at DldIOService.cpp:364  
DldHookerCommonClass::open at DldHookerCommonClass.cpp:621  
IOServiceVtableDldHookDldInheritanceDepth_0::open_hook at IOServiceDldHook.cpp:20  
IOStorage::open at IOStorage.cpp:216  
IOApplePartitionScheme::scan at IOApplePartitionScheme.cpp:258  
IOApplePartitionScheme::probe at IOApplePartitionScheme.cpp:101  
IOService::probeCandidates at IOService.cpp:2702  
IOService::doServiceMatch at IOService.cpp:3088  
_IOConfigThread::main at IOService.cpp:3350  
```

##Usage

The project contains an example of hooker usage for IOUserClient objects.

For particular details look at HookExample function that initiates hooking for IOUserClient derived objects.

Below is a brief description for IOUserClient hook and DldIOKitHookEngine. DldIOKitHookEngine is a class that registeres and applies hooks by traversing the IOService plane. You can extend this example by adding dynamic hooking for newly created objects, see MacOSX-Kernel-Filter for details.

The hooking is performed by 
```
IOReturn
DldIOKitHookEngine::HookObject(
    __inout OSObject* object
    )
```

which applies a hooking function for each registered class

```
( pEntry->getHookFunction() )( object, pEntry->getHookType() );
```

The template class

```
template<DldInheritanceDepth Depth>
class IOUserClientDldHook : public DldHookerBaseInterface
```

defines a hooker for IOUserClient derived classes up to Depth of inheritance. This template is instantiated by DldIOKitHookEngine::startHookingWithPredefinedClasses as

```
    this->DldAddHookingClassInstance< IOUserClientDldHook<DldInheritanceDepth_0>, IOUserClient >( DldHookTypeVtable );
    this->DldAddHookingClassInstance< IOUserClientDldHook<DldInheritanceDepth_1>, IOUserClient >( DldHookTypeVtable );
    this->DldAddHookingClassInstance< IOUserClientDldHook<DldInheritanceDepth_2>, IOUserClient >( DldHookTypeVtable );
    this->DldAddHookingClassInstance< IOUserClientDldHook<DldInheritanceDepth_3>, IOUserClient >( DldHookTypeVtable );
    this->DldAddHookingClassInstance< IOUserClientDldHook<DldInheritanceDepth_4>, IOUserClient >( DldHookTypeVtable );
    this->DldAddHookingClassInstance< IOUserClientDldHook<DldInheritanceDepth_5>, IOUserClient >( DldHookTypeVtable );
    this->DldAddHookingClassInstance< IOUserClientDldHook<DldInheritanceDepth_6>, IOUserClient >( DldHookTypeVtable );
    this->DldAddHookingClassInstance< IOUserClientDldHook<DldInheritanceDepth_7>, IOUserClient >( DldHookTypeVtable );
    this->DldAddHookingClassInstance< IOUserClientDldHook<DldInheritanceDepth_8>, IOUserClient >( DldHookTypeVtable );
    this->DldAddHookingClassInstance< IOUserClientDldHook<DldInheritanceDepth_9>, IOUserClient >( DldHookTypeVtable );
    this->DldAddHookingClassInstance< IOUserClientDldHook<DldInheritanceDepth_10>, IOUserClient >( DldHookTypeVtable );
```

the static object of a class is created by a call to fStaticContainerClassInstance

```
if( !DldHookerCommonClass2<CC,HC>::fStaticContainerClassInstance() ){
```

this static object is used to hook IOUserClient inherited objects by providing hooking functions registered by init() routine

```
template<DldInheritanceDepth Depth>
bool
IOUserClientDldHook<Depth>::init()
{
    // super::init()
    
    assert( this->mHookerCommon2 );
    
    if( this->mHookerCommon2->init( this ) ){
        
        //
        // add new hooking functions specific for the class
        //
        this->mHookerCommon2->fAddHookingFunctionExternal(
                                                          IOUserClientDldHook<Depth>::kDld_getExternalMethodForIndex_hook,
                                                          DldConvertFunctionToVtableIndex( (void (OSMetaClassBase::*)(void)) &IOUserClient::getExternalMethodForIndex ),
                                                          DldHookerCommonClass2<IOUserClientDldHook<Depth>,IOUserClient>::_ptmf2ptf( this, (void (DldHookerBaseInterface::*)(void)) &IOUserClientDldHook<Depth>::getExternalMethodForIndex_hook ) );
        

        this->mHookerCommon2->fAddHookingFunctionExternal(
                                                          IOUserClientDldHook<Depth>::kDld_getExternalAsyncMethodForIndex_hook,
                                                          DldConvertFunctionToVtableIndex( (void (OSMetaClassBase::*)(void)) &IOUserClient::getExternalAsyncMethodForIndex ),
                                                          DldHookerCommonClass2<IOUserClientDldHook<Depth>,IOUserClient>::_ptmf2ptf( this, (void (DldHookerBaseInterface::*)(void)) &IOUserClientDldHook<Depth>::getExternalAsyncMethodForIndex_hook ) );
        

        this->mHookerCommon2->fAddHookingFunctionExternal(
                                                          IOUserClientDldHook<Depth>::kDld_getTargetAndMethodForIndex_hook,
                                                          DldConvertFunctionToVtableIndex( (void (OSMetaClassBase::*)(void)) &IOUserClient::getTargetAndMethodForIndex ),
                                                          DldHookerCommonClass2<IOUserClientDldHook<Depth>,IOUserClient>::_ptmf2ptf( this, (void (DldHookerBaseInterface::*)(void)) &IOUserClientDldHook<Depth>::getTargetAndMethodForIndex_hook ) );
        

.....
```

The enumerator type defines index for each virtual function hook
```
    enum{
        kDld_getExternalMethodForIndex_hook = 0x0,
        kDld_getExternalAsyncMethodForIndex_hook,
        kDld_getTargetAndMethodForIndex_hook,
        kDld_getAsyncTargetAndMethodForIndex_hook,
        kDld_getExternalTrapForIndex_hook,
        kDld_getTargetAndTrapForIndex_hook,
        kDld_externalMethod_hook,
        kDld_clientMemoryForType_hook,
        kDld_registerNotificationPort1_hook,//(mach_port_t port, UInt32 type, io_user_reference_t refCon)
        kDld_registerNotificationPort2_hook,//(mach_port_t port, UInt32 type, UInt32 refCon )
        kDld_getNotificationSemaphore_hook,
        kDld_NumberOfAddedHooks
    };
```

Then goes a definition for each hooking function
```
    // Old methods for accessing method vector backward compatiblility only
    virtual IOExternalMethod * getExternalMethodForIndex_hook( UInt32 index );
    
    virtual IOExternalAsyncMethod * getExternalAsyncMethodForIndex_hook( UInt32 index );
    
    // Methods for accessing method vector.
    virtual IOExternalMethod * getTargetAndMethodForIndex_hook( IOService ** targetP, UInt32 index );
    
    virtual IOExternalAsyncMethod * getAsyncTargetAndMethodForIndex_hook( IOService ** targetP, UInt32 index );
    
    // Methods for accessing trap vector - old and new style
    virtual IOExternalTrap * getExternalTrapForIndex_hook( UInt32 index );
    
    virtual IOExternalTrap * getTargetAndTrapForIndex_hook( IOService **targetP, UInt32 index );
    
    virtual IOReturn externalMethod_hook( uint32_t selector, IOExternalMethodArguments * arguments,
                                    IOExternalMethodDispatch * dispatch = 0, OSObject * target = 0, void * reference = 0 );
    
    virtual IOReturn registerNotificationPort1_hook(mach_port_t port, UInt32 type, io_user_reference_t refCon);
    
    virtual IOReturn registerNotificationPort2_hook(mach_port_t port, UInt32 type, UInt32 refCon );
    
    virtual IOReturn clientMemoryForType_hook( UInt32 type,
                                               IOOptionBits * options,
                                               IOMemoryDescriptor ** memory );
    
    virtual IOReturn getNotificationSemaphore_hook( UInt32 notification_type,
                                                    semaphore_t * semaphore );
```

The actual object hooking is performed by an instance of DldHookerCommonClass2 class template

```
DldHookerCommonClass2<IOUserClientDldHook<Depth>,IOUserClient>*   mHookerCommon2;
```
which provides fHookObject function

```
template <class CC, class HC>
IOReturn
DldHookerCommonClass2<CC,HC>::fHookObject( __inout OSObject* object, __in DldHookType type )
{
    DldHookerCommonClass2<CC,HC>*  commonHooker2;
    
    commonHooker2 = DldHookerCommonClass2<CC,HC>::fCommonHooker2();
    assert( commonHooker2 );
    if( NULL == commonHooker2 )
        return false;
    
    return commonHooker2->mHookerCommon.HookObject( object, type );
}
```

You need to provide a template class similar for IOUserClientDldHook for each IOKit class you want to hook. For example below is a class declaration for IOUSBMassStorageClass

```
template<DldInheritanceDepth Depth>
class IOUSBMassStorageClassDldHook2 : public DldHookerBaseInterface
{

    /////////////////////////////////////////////
    //
    // start of the required declarations
    //
    /////////////////////////////////////////////
public:
    enum{
        kDld_SendSCSICommand_hook = 0x0,
        kDld_NumberOfAddedHooks
    };
    
    friend class DldIOKitHookEngine;
    friend class DldHookerCommonClass2<IOUSBMassStorageClassDldHook2<Depth>,IOUSBMassStorageClass>;
    
    static const char* fGetHookedClassName(){ return "IOUSBMassStorageClass"; };
    DldDeclareGetClassNameFunction();
    
protected:
    
    static DldInheritanceDepth fGetInheritanceDepth(){ return Depth; };
    DldHookerCommonClass2<IOUSBMassStorageClassDldHook2<Depth>,IOUSBMassStorageClass>*   mHookerCommon2;
    
protected:
    static IOUSBMassStorageClassDldHook2<Depth>* newWithDefaultSettings();
    virtual bool init();
    virtual void free();
    
    /////////////////////////////////////////////
    //
    // end of the required declarations
    //
    /////////////////////////////////////////////
    
protected:
    
    //
    // The SendSCSICommand function will take a SCSITask Object and transport
	// it across the physical wire(s) to the device
    //
	virtual bool SendSCSICommand_hook( SCSITaskIdentifier 		request, 
                                       SCSIServiceResponse *	serviceResponse,
                                       SCSITaskStatus	   *	taskStatus );
    
};
```
