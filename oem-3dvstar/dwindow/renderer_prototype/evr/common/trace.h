//////////////////////////////////////////////////////////////////////////
//
// trace.h : Functions to return the names of constants.
// 
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//
//////////////////////////////////////////////////////////////////////////

#pragma once

#include "logging.h"
#include <mfidl.h>

#ifndef NAMET
#define NAMET(x) case x: return L#x
#endif

namespace MediaFoundationSamples
{

    // IMPORTANT: No function here can return a NULL pointer - caller assumes
    // the return value is a valid null-terminated string. You should only
    // use these functions for debugging purposes.

    // Media Foundation event names (subset)
    inline const WCHAR* EventName(MediaEventType met)
    {
        switch (met)
        {
            NAMET(MEError);
            NAMET(MEExtendedType);
            NAMET(MESessionTopologySet);
            NAMET(MESessionTopologiesCleared);
            NAMET(MESessionStarted);
            NAMET(MESessionPaused);
            NAMET(MESessionStopped);
            NAMET(MESessionClosed);
            NAMET(MESessionEnded);
            NAMET(MESessionRateChanged);
            NAMET(MESessionScrubSampleComplete);
            NAMET(MESessionCapabilitiesChanged);
            NAMET(MESessionTopologyStatus);
            NAMET(MESessionNotifyPresentationTime);
            NAMET(MENewPresentation);
            NAMET(MELicenseAcquisitionStart);
            NAMET(MELicenseAcquisitionCompleted);
            NAMET(MEIndividualizationStart);
            NAMET(MEIndividualizationCompleted);
            NAMET(MEEnablerProgress);
            NAMET(MEEnablerCompleted);
            NAMET(MEPolicyError);
            NAMET(MEPolicyReport);
            NAMET(MEBufferingStarted);
            NAMET(MEBufferingStopped);
            NAMET(MEConnectStart);
            NAMET(MEConnectEnd);
            NAMET(MEReconnectStart);
            NAMET(MEReconnectEnd);
            NAMET(MERendererEvent);
            NAMET(MESessionStreamSinkFormatChanged);
            NAMET(MESourceStarted);
            NAMET(MEStreamStarted);
            NAMET(MESourceSeeked);
            NAMET(MEStreamSeeked);
            NAMET(MENewStream);
            NAMET(MEUpdatedStream);
            NAMET(MESourceStopped);
            NAMET(MEStreamStopped);
            NAMET(MESourcePaused);
            NAMET(MEStreamPaused);
            NAMET(MEEndOfPresentation);
            NAMET(MEEndOfStream);
            NAMET(MEMediaSample);
            NAMET(MEStreamTick);
            NAMET(MEStreamThinMode);
            NAMET(MEStreamFormatChanged);
            NAMET(MESourceRateChanged);
            NAMET(MEEndOfPresentationSegment);
            NAMET(MESourceCharacteristicsChanged);
            NAMET(MESourceRateChangeRequested);
            NAMET(MESourceMetadataChanged);
            NAMET(MESequencerSourceTopologyUpdated);
            NAMET(MEStreamSinkStarted);
            NAMET(MEStreamSinkStopped);
            NAMET(MEStreamSinkPaused);
            NAMET(MEStreamSinkRateChanged);
            NAMET(MEStreamSinkRequestSample);
            NAMET(MEStreamSinkMarker);
            NAMET(MEStreamSinkPrerolled);
            NAMET(MEStreamSinkScrubSampleComplete);
            NAMET(MEStreamSinkFormatChanged);
            NAMET(MEStreamSinkDeviceChanged);
            NAMET(MEQualityNotify);
            NAMET(MESinkInvalidated);
            NAMET(MEAudioSessionNameChanged);
            NAMET(MEAudioSessionVolumeChanged);
            NAMET(MEAudioSessionDeviceRemoved);
            NAMET(MEAudioSessionServerShutdown);
            NAMET(MEAudioSessionGroupingParamChanged);
            NAMET(MEAudioSessionIconChanged);
            NAMET(MEAudioSessionFormatChanged);
            NAMET(MEAudioSessionDisconnected);
            NAMET(MEAudioSessionExclusiveModeOverride);
            NAMET(MEPolicyChanged);
            NAMET(MEContentProtectionMessage);
            NAMET(MEPolicySet);

        default:
            return L"Unknown event";
        }
    }

    // Names of VARIANT data types. 
    inline const WCHAR* VariantTypeName(const PROPVARIANT& prop)
    {
        switch (prop.vt & VT_TYPEMASK)
        {
            NAMET(VT_EMPTY);
            NAMET(VT_NULL);
            NAMET(VT_I2);
            NAMET(VT_I4);
            NAMET(VT_R4);
            NAMET(VT_R8);
            NAMET(VT_CY);
            NAMET(VT_DATE);
            NAMET(VT_BSTR);
            NAMET(VT_DISPATCH);
            NAMET(VT_ERROR);
            NAMET(VT_BOOL);
            NAMET(VT_VARIANT);
            NAMET(VT_UNKNOWN);
            NAMET(VT_DECIMAL);
            NAMET(VT_I1);
            NAMET(VT_UI1);
            NAMET(VT_UI2);
            NAMET(VT_UI4);
            NAMET(VT_I8);
            NAMET(VT_UI8);
            NAMET(VT_INT);
            NAMET(VT_UINT);
            NAMET(VT_VOID);
            NAMET(VT_HRESULT);
            NAMET(VT_PTR);
            NAMET(VT_SAFEARRAY);
            NAMET(VT_CARRAY);
            NAMET(VT_USERDEFINED);
            NAMET(VT_LPSTR);
            NAMET(VT_LPWSTR);
            NAMET(VT_RECORD);
            NAMET(VT_INT_PTR);
            NAMET(VT_UINT_PTR);
            NAMET(VT_FILETIME);
            NAMET(VT_BLOB);
            NAMET(VT_STREAM);
            NAMET(VT_STORAGE);
            NAMET(VT_STREAMED_OBJECT);
            NAMET(VT_STORED_OBJECT);
            NAMET(VT_BLOB_OBJECT);
            NAMET(VT_CF);
            NAMET(VT_CLSID);
            NAMET(VT_VERSIONED_STREAM);
        default:
            return L"Unknown VARIANT type";
        }
    }

    // Names of topology node types.
    inline const WCHAR* TopologyNodeTypeName(MF_TOPOLOGY_TYPE nodeType)
    {
        switch (nodeType)
        {
            NAMET(MF_TOPOLOGY_OUTPUT_NODE);
            NAMET(MF_TOPOLOGY_SOURCESTREAM_NODE);
            NAMET(MF_TOPOLOGY_TRANSFORM_NODE);
            NAMET(MF_TOPOLOGY_TEE_NODE);
        default:
            return L"Unknown node type";
        }
    }

    inline const WCHAR* MFTMessageName(MFT_MESSAGE_TYPE msg)
    {
        switch (msg)
        {
            NAMET(MFT_MESSAGE_COMMAND_FLUSH);
            NAMET(MFT_MESSAGE_COMMAND_DRAIN);
            NAMET(MFT_MESSAGE_SET_D3D_MANAGER);
            NAMET(MFT_MESSAGE_NOTIFY_BEGIN_STREAMING);
            NAMET(MFT_MESSAGE_NOTIFY_END_STREAMING);
            NAMET(MFT_MESSAGE_NOTIFY_END_OF_STREAM);
            NAMET(MFT_MESSAGE_NOTIFY_START_OF_STREAM);
        default:
            return L"Unknown message";
        }
    }

}; // namespace MediaFoundationSamples
