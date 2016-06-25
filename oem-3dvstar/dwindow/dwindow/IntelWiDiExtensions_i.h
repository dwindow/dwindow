

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 7.00.0555 */
/* at Fri Jun 24 14:38:58 2011
 */
/* Compiler settings for IntelWiDiExtensions.idl:
    Oicf, W1, Zp8, env=Win32 (32b run), target_arch=X86 7.00.0555 
    protocol : dce , ms_ext, c_ext, robust
    error checks: allocation ref bounds_check enum stub_data 
    VC __declspec() decoration level: 
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
*/
/* @@MIDL_FILE_HEADING(  ) */

#pragma warning( disable: 4049 )  /* more than 64k source lines */


/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 475
#endif

#include "rpc.h"
#include "rpcndr.h"

#ifndef __RPCNDR_H_VERSION__
#error this stub requires an updated version of <rpcndr.h>
#endif // __RPCNDR_H_VERSION__

#ifndef COM_NO_WINDOWS_H
#include "windows.h"
#include "ole2.h"
#endif /*COM_NO_WINDOWS_H*/

#ifndef __IntelWiDiExtensions_i_h__
#define __IntelWiDiExtensions_i_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IWiDiExtensions_FWD_DEFINED__
#define __IWiDiExtensions_FWD_DEFINED__
typedef interface IWiDiExtensions IWiDiExtensions;
#endif 	/* __IWiDiExtensions_FWD_DEFINED__ */


#ifndef __WiDiExtensions_FWD_DEFINED__
#define __WiDiExtensions_FWD_DEFINED__

#ifdef __cplusplus
typedef class WiDiExtensions WiDiExtensions;
#else
typedef struct WiDiExtensions WiDiExtensions;
#endif /* __cplusplus */

#endif 	/* __WiDiExtensions_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"

#ifdef __cplusplus
extern "C"{
#endif 


#ifndef __IWiDiExtensions_INTERFACE_DEFINED__
#define __IWiDiExtensions_INTERFACE_DEFINED__

/* interface IWiDiExtensions */
/* [unique][helpstring][nonextensible][dual][uuid][object] */ 

typedef 
enum SM
    {	Default	= 0,
	LocalOnly	= ( Default + 1 ) ,
	Clone	= ( LocalOnly + 1 ) ,
	Extended	= ( Clone + 1 ) ,
	ExternalOnly	= ( Extended + 1 ) ,
	Invalid	= ( ExternalOnly + 1 ) 
    } 	ScreenMode;


EXTERN_C const IID IID_IWiDiExtensions;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("D8D15998-D42E-4FC7-A7B9-6529CE34301A")
    IWiDiExtensions : public IDispatch
    {
    public:
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE GetAPIVersion( 
            /* [out] */ BSTR *bsVersionNumber) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE GetHostCapability( 
            /* [in] */ BSTR bsKey,
            /* [out] */ BSTR *pbsValue) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Initialize( 
            /* [in] */ DWORD hCallbackWindow) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Shutdown( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE StartScanForAdapters( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE StartConnectionToAdapter( 
            /* [in] */ BSTR bsAdapterId,
            /* [in] */ int nSrcScreenRes,
            /* [in] */ int nTgtScreenRes,
            /* [in] */ ScreenMode Mode) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE DisconnectAdapter( 
            /* [in] */ BSTR bsAdapterId) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE SetScreenMode( 
            /* [in] */ ScreenMode nScreenMode) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE GetHostStatus( 
            /* [in] */ BSTR bsKey,
            /* [out] */ BSTR *pbsValue) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE SupplyAuthenticationInput( 
            /* [in] */ BSTR bsKey) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE GetAdapterInformation( 
            /* [in] */ BSTR bsAdapterID,
            /* [in] */ BSTR bsKey,
            /* [out] */ BSTR *pbsValue) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE GetAdapterList( 
            /* [in] */ BSTR bsFilter,
            /* [in] */ BSTR bsType,
            /* [out] */ BSTR *pbsAdapterList) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWiDiExtensionsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IWiDiExtensions * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            __RPC__deref_out  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IWiDiExtensions * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IWiDiExtensions * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IWiDiExtensions * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IWiDiExtensions * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IWiDiExtensions * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [range][in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IWiDiExtensions * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *GetAPIVersion )( 
            IWiDiExtensions * This,
            /* [out] */ BSTR *bsVersionNumber);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *GetHostCapability )( 
            IWiDiExtensions * This,
            /* [in] */ BSTR bsKey,
            /* [out] */ BSTR *pbsValue);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Initialize )( 
            IWiDiExtensions * This,
            /* [in] */ DWORD hCallbackWindow);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Shutdown )( 
            IWiDiExtensions * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *StartScanForAdapters )( 
            IWiDiExtensions * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *StartConnectionToAdapter )( 
            IWiDiExtensions * This,
            /* [in] */ BSTR bsAdapterId,
            /* [in] */ int nSrcScreenRes,
            /* [in] */ int nTgtScreenRes,
            /* [in] */ ScreenMode Mode);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *DisconnectAdapter )( 
            IWiDiExtensions * This,
            /* [in] */ BSTR bsAdapterId);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *SetScreenMode )( 
            IWiDiExtensions * This,
            /* [in] */ ScreenMode nScreenMode);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *GetHostStatus )( 
            IWiDiExtensions * This,
            /* [in] */ BSTR bsKey,
            /* [out] */ BSTR *pbsValue);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *SupplyAuthenticationInput )( 
            IWiDiExtensions * This,
            /* [in] */ BSTR bsKey);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *GetAdapterInformation )( 
            IWiDiExtensions * This,
            /* [in] */ BSTR bsAdapterID,
            /* [in] */ BSTR bsKey,
            /* [out] */ BSTR *pbsValue);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *GetAdapterList )( 
            IWiDiExtensions * This,
            /* [in] */ BSTR bsFilter,
            /* [in] */ BSTR bsType,
            /* [out] */ BSTR *pbsAdapterList);
        
        END_INTERFACE
    } IWiDiExtensionsVtbl;

    interface IWiDiExtensions
    {
        CONST_VTBL struct IWiDiExtensionsVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IWiDiExtensions_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IWiDiExtensions_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IWiDiExtensions_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IWiDiExtensions_GetTypeInfoCount(This,pctinfo)	\
    ( (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo) ) 

#define IWiDiExtensions_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    ( (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo) ) 

#define IWiDiExtensions_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    ( (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId) ) 

#define IWiDiExtensions_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    ( (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr) ) 


#define IWiDiExtensions_GetAPIVersion(This,bsVersionNumber)	\
    ( (This)->lpVtbl -> GetAPIVersion(This,bsVersionNumber) ) 

#define IWiDiExtensions_GetHostCapability(This,bsKey,pbsValue)	\
    ( (This)->lpVtbl -> GetHostCapability(This,bsKey,pbsValue) ) 

#define IWiDiExtensions_Initialize(This,hCallbackWindow)	\
    ( (This)->lpVtbl -> Initialize(This,hCallbackWindow) ) 

#define IWiDiExtensions_Shutdown(This)	\
    ( (This)->lpVtbl -> Shutdown(This) ) 

#define IWiDiExtensions_StartScanForAdapters(This)	\
    ( (This)->lpVtbl -> StartScanForAdapters(This) ) 

#define IWiDiExtensions_StartConnectionToAdapter(This,bsAdapterId,nSrcScreenRes,nTgtScreenRes,Mode)	\
    ( (This)->lpVtbl -> StartConnectionToAdapter(This,bsAdapterId,nSrcScreenRes,nTgtScreenRes,Mode) ) 

#define IWiDiExtensions_DisconnectAdapter(This,bsAdapterId)	\
    ( (This)->lpVtbl -> DisconnectAdapter(This,bsAdapterId) ) 

#define IWiDiExtensions_SetScreenMode(This,nScreenMode)	\
    ( (This)->lpVtbl -> SetScreenMode(This,nScreenMode) ) 

#define IWiDiExtensions_GetHostStatus(This,bsKey,pbsValue)	\
    ( (This)->lpVtbl -> GetHostStatus(This,bsKey,pbsValue) ) 

#define IWiDiExtensions_SupplyAuthenticationInput(This,bsKey)	\
    ( (This)->lpVtbl -> SupplyAuthenticationInput(This,bsKey) ) 

#define IWiDiExtensions_GetAdapterInformation(This,bsAdapterID,bsKey,pbsValue)	\
    ( (This)->lpVtbl -> GetAdapterInformation(This,bsAdapterID,bsKey,pbsValue) ) 

#define IWiDiExtensions_GetAdapterList(This,bsFilter,bsType,pbsAdapterList)	\
    ( (This)->lpVtbl -> GetAdapterList(This,bsFilter,bsType,pbsAdapterList) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IWiDiExtensions_INTERFACE_DEFINED__ */



#ifndef __IntelWiDiLib_LIBRARY_DEFINED__
#define __IntelWiDiLib_LIBRARY_DEFINED__

/* library IntelWiDiLib */
/* [helpstring][version][uuid] */ 

#define	WM_WIDI_MESSAGE_BASE	( 0x9000 )

#define	WIDI_SUCCESS	( 0 )

#define	WIDI_ERR	( ( ( ( 1 << 31 )  | ( 4 << 16 )  )  | 0x9000 )  )

/* [v1_enum][uuid] */ 
enum  DECLSPEC_UUID("5514FC70-4A65-40d0-9A4A-C1B360CEA5A3") ReturnCode
    {	S_SUCCESS	= WIDI_SUCCESS,
	S_ALREADY_INITIALIZED	= ( WIDI_SUCCESS + 1 ) ,
	E_NOT_INITIALIZED	= ( WIDI_ERR + 1 ) ,
	E_WIDI_AGENT_NOT_FOUND	= ( WIDI_ERR + 2 ) ,
	E_NOT_CONNECTED	= ( WIDI_ERR + 3 ) ,
	E_ADAPTER_NOT_FOUND	= ( WIDI_ERR + 4 ) ,
	E_INTERNAL_ERROR	= ( WIDI_ERR + 5 ) ,
	E_INVALID_PARAMETER	= ( WIDI_ERR + 6 ) ,
	E_CONNECTED	= ( WIDI_ERR + 7 ) ,
	E_KEY_NOT_SUPPORTED	= ( WIDI_ERR + 8 ) ,
	E_NOT_IMPL	= ( WIDI_ERR + 9 ) ,
	E_NO_SCAN_WHILE_CONNECTED	= ( WIDI_ERR + 10 ) ,
	E_MUST_SCAN	= ( WIDI_ERR + 11 ) ,
	E_ALREADY_INITIALIZED	= ( WIDI_ERR + 12 ) ,
	E_INVALID_WIDIVERSION	= ( WIDI_ERR + 13 ) ,
	E_SCAN_IN_PROGRESS	= ( WIDI_ERR + 14 ) ,
	E_UNABLE_TO_START_SCAN	= ( WIDI_ERR + 15 ) ,
	E_NO_DEFAULT_ADAPTER	= ( WIDI_ERR + 16 ) ,
	E_WIDI_APPLICATION_ERROR	= ( WIDI_ERR + 17 ) ,
	E_INITIALIZING	= ( WIDI_ERR + 18 ) ,
	E_ADAPTER_TYPE	= ( WIDI_ERR + 19 ) ,
	E_INVALID_DISPLAY_TOPOLOGY	= ( WIDI_ERR + 20 ) 
    } ;
/* [v1_enum][uuid] */ 
enum  DECLSPEC_UUID("F6B3E34B-93E3-4f43-A8BD-2BE59164C29D") ReasonCode
    {	RC_SUCCESS	= 0,
	RC_UNKNOWN	= ( RC_SUCCESS + 1 ) ,
	RC_ADAPTER_NOT_FOUND	= ( RC_UNKNOWN + 1 ) ,
	RC_CONNECTION_CANCELLED	= ( RC_ADAPTER_NOT_FOUND + 1 ) ,
	RC_USER_DISCONNECTED	= ( RC_CONNECTION_CANCELLED + 1 ) ,
	RC_CONNECTION_DROPPED	= ( RC_USER_DISCONNECTED + 1 ) ,
	RC_WIDI_APP_NOT_FOUND	= ( RC_CONNECTION_DROPPED + 1 ) ,
	RC_WIDI_FAILED_TO_START	= ( RC_WIDI_APP_NOT_FOUND + 1 ) ,
	RC_WIDI_FAILED_TO_CONNECT	= ( RC_WIDI_FAILED_TO_START + 1 ) ,
	RC_WIDI_FAILED_TO_DISCONNECT	= ( RC_WIDI_FAILED_TO_CONNECT + 1 ) ,
	RC_ALREADY_CONNECTED	= ( RC_WIDI_FAILED_TO_DISCONNECT + 1 ) ,
	RC_NOT_CONNECTED	= ( RC_ALREADY_CONNECTED + 1 ) ,
	RC_SCAN_IN_PROGRESS	= ( RC_NOT_CONNECTED + 1 ) ,
	RC_UNABLE_TO_START_SCAN	= ( RC_SCAN_IN_PROGRESS + 1 ) ,
	RC_NO_DEFAULT_ADAPTER	= ( RC_UNABLE_TO_START_SCAN + 1 ) ,
	RC_WIDI_APPLICATION_ERROR	= ( RC_NO_DEFAULT_ADAPTER + 1 ) ,
	RC_CONNECT_CANCELLED_SCAN	= ( RC_WIDI_APPLICATION_ERROR + 1 ) ,
	RC_INTERNAL_ERROR	= ( RC_CONNECT_CANCELLED_SCAN + 1 ) 
    } ;
/* [v1_enum][uuid] */ 
enum  DECLSPEC_UUID("F6B3E34B-93E3-4f43-A8BD-2BE59164C29E") WiDiMessages
    {	WM_WIDI_INITIALIZED	= WM_WIDI_MESSAGE_BASE,
	WM_WIDI_INITIALIZATION_FAILED	= ( WM_WIDI_MESSAGE_BASE + 1 ) ,
	WM_WIDI_ADAPTER_DISCOVERED	= ( WM_WIDI_MESSAGE_BASE + 2 ) ,
	WM_WIDI_SCAN_COMPLETE	= ( WM_WIDI_MESSAGE_BASE + 3 ) ,
	WM_WIDI_NEEDS_AUTH_INPUT	= ( WM_WIDI_MESSAGE_BASE + 4 ) ,
	WM_WIDI_CONNECTION_FAILED	= ( WM_WIDI_MESSAGE_BASE + 5 ) ,
	WM_WIDI_CONNECTED	= ( WM_WIDI_MESSAGE_BASE + 6 ) ,
	WM_WIDI_DISCONNECTED	= ( WM_WIDI_MESSAGE_BASE + 7 ) ,
	WM_WIDI_DISCONNECT_FAILED	= ( WM_WIDI_MESSAGE_BASE + 8 ) 
    } ;

EXTERN_C const IID LIBID_IntelWiDiLib;

EXTERN_C const CLSID CLSID_WiDiExtensions;

#ifdef __cplusplus

class DECLSPEC_UUID("7DC2B7AA-BCFD-44D2-BD58-E8BD0D2E3ACC")
WiDiExtensions;
#endif
#endif /* __IntelWiDiLib_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

unsigned long             __RPC_USER  BSTR_UserSize(     unsigned long *, unsigned long            , BSTR * ); 
unsigned char * __RPC_USER  BSTR_UserMarshal(  unsigned long *, unsigned char *, BSTR * ); 
unsigned char * __RPC_USER  BSTR_UserUnmarshal(unsigned long *, unsigned char *, BSTR * ); 
void                      __RPC_USER  BSTR_UserFree(     unsigned long *, BSTR * ); 

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


