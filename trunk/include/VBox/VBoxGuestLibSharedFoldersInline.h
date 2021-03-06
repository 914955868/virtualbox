/* $Id$ */
/** @file
 * VBoxGuestLib - Shared Folders Host Request Helpers (ring-0).
 */

/*
 * Copyright (C) 2006-2019 Oracle Corporation
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT.  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef VBOX_INCLUDED_VBoxGuestLibSharedFoldersInline_h
#define VBOX_INCLUDED_VBoxGuestLibSharedFoldersInline_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <iprt/types.h>
#include <iprt/assert.h>
#include <VBox/VBoxGuest.h>
#include <VBox/VBoxGuestLib.h>
#include <VBox/VBoxGuestLibSharedFolders.h>
#include <VBox/VMMDev.h>
#include <VBox/shflsvc.h>
#include <iprt/errcore.h>


/** @defgroup grp_vboxguest_lib_r0_sf_inline    Shared Folders Host Request Helpers
 * @ingroup grp_vboxguest_lib_r0
 *
 * @note Using inline functions to avoid wasting precious ring-0 stack space on
 *       passing parameters that ends up in the structure @a pReq points to.  It
 *       is also safe to assume that it's faster too.  It's worth a few bytes
 *       larger code section in the resulting shared folders driver.
 *
 * @note This currently requires a C++ compiler or a C compiler capable of
 *       mixing code and variables (i.e. C99).
 *
 * @{
 */

/** VMMDEV_HVF_XXX (set during init). */
extern uint32_t g_fHostFeatures;
extern VBGLSFCLIENT g_SfClient; /**< Move this into the parameters? */

/** Request structure for VbglR0SfHostReqMapFolderWithBuf.  */
typedef struct VBOXSFMAPFOLDERWITHBUFREQ
{
    VBGLIOCIDCHGCMFASTCALL  Hdr;
    VMMDevHGCMCall          Call;
    VBoxSFParmMapFolder     Parms;
    HGCMPageListInfo        PgLst;
} VBOXSFMAPFOLDERWITHBUFREQ;

/**
 * SHFL_FN_MAP_FOLDER request.
 */
DECLINLINE(int) VbglR0SfHostReqMapFolderWithBuf(VBOXSFMAPFOLDERWITHBUFREQ *pReq, PSHFLSTRING pStrName,
                                                RTUTF16 wcDelimiter, bool fCaseSensitive)
{
    VBGLIOCIDCHGCMFASTCALL_INIT(&pReq->Hdr, VbglR0PhysHeapGetPhysAddr(pReq), &pReq->Call, g_SfClient.idClient,
                                SHFL_FN_MAP_FOLDER, SHFL_CPARMS_MAP_FOLDER, sizeof(*pReq));

    pReq->Parms.id32Root.type                   = VMMDevHGCMParmType_32bit;
    pReq->Parms.id32Root.u.value32              = SHFL_ROOT_NIL;

    pReq->Parms.uc32Delimiter.type              = VMMDevHGCMParmType_32bit;
    pReq->Parms.uc32Delimiter.u.value32         = wcDelimiter;

    pReq->Parms.fCaseSensitive.type             = VMMDevHGCMParmType_32bit;
    pReq->Parms.fCaseSensitive.u.value32        = fCaseSensitive;

    if (g_fHostFeatures & VMMDEV_HVF_HGCM_CONTIGUOUS_PAGE_LIST)
    {
        pReq->Parms.pStrName.type               = VMMDevHGCMParmType_PageList;
        pReq->Parms.pStrName.u.PageList.size    = SHFLSTRING_HEADER_SIZE + pStrName->u16Size;
        pReq->Parms.pStrName.u.PageList.offset  = RT_UOFFSETOF(VBOXSFMAPFOLDERWITHBUFREQ, PgLst) - sizeof(VBGLIOCIDCHGCMFASTCALL);
        pReq->PgLst.flags                       = VBOX_HGCM_F_PARM_DIRECTION_BOTH;
        pReq->PgLst.aPages[0]                   = VbglR0PhysHeapGetPhysAddr(pStrName);
        pReq->PgLst.offFirstPage                = (uint16_t)(pReq->PgLst.aPages[0] & PAGE_OFFSET_MASK);
        pReq->PgLst.aPages[0]                  &= ~(RTGCPHYS)PAGE_OFFSET_MASK;
        pReq->PgLst.cPages                      = 1;
    }
    else
    {
        pReq->Parms.pStrName.type               = VMMDevHGCMParmType_LinAddr_In;
        pReq->Parms.pStrName.u.LinAddr.cb       = SHFLSTRING_HEADER_SIZE + pStrName->u16Size;
        pReq->Parms.pStrName.u.LinAddr.uAddr    = (uintptr_t)pStrName;
    }

    int vrc = VbglR0HGCMFastCall(g_SfClient.handle, &pReq->Hdr, sizeof(*pReq));
    if (RT_SUCCESS(vrc))
        vrc = pReq->Call.header.result;
    return vrc;
}



/** Request structure used by vboxSfOs2HostReqUnmapFolder.  */
typedef struct VBOXSFUNMAPFOLDERREQ
{
    VBGLIOCIDCHGCMFASTCALL  Hdr;
    VMMDevHGCMCall          Call;
    VBoxSFParmUnmapFolder   Parms;
} VBOXSFUNMAPFOLDERREQ;


/**
 * SHFL_FN_UNMAP_FOLDER request.
 */
DECLINLINE(int) VbglR0SfHostReqUnmapFolderSimple(uint32_t idRoot)
{
    VBOXSFUNMAPFOLDERREQ *pReq = (VBOXSFUNMAPFOLDERREQ *)VbglR0PhysHeapAlloc(sizeof(*pReq));
    if (pReq)
    {
        pReq->Parms.id32Root.type      = VMMDevHGCMParmType_32bit;
        pReq->Parms.id32Root.u.value32 = idRoot;

        VBGLIOCIDCHGCMFASTCALL_INIT(&pReq->Hdr, VbglR0PhysHeapGetPhysAddr(pReq), &pReq->Call, g_SfClient.idClient,
                                    SHFL_FN_UNMAP_FOLDER, SHFL_CPARMS_UNMAP_FOLDER, sizeof(*pReq));

        int vrc = VbglR0HGCMFastCall(g_SfClient.handle, &pReq->Hdr, sizeof(*pReq));
        if (RT_SUCCESS(vrc))
            vrc = pReq->Call.header.result;

        VbglR0PhysHeapFree(pReq);
        return vrc;
    }
    return VERR_NO_MEMORY;
}


/** Request structure for VbglR0SfHostReqCreate.  */
typedef struct VBOXSFCREATEREQ
{
    VBGLIOCIDCHGCMFASTCALL  Hdr;
    VMMDevHGCMCall          Call;
    VBoxSFParmCreate        Parms;
    SHFLCREATEPARMS         CreateParms;
    SHFLSTRING              StrPath;
} VBOXSFCREATEREQ;

/**
 * SHFL_FN_CREATE request.
 */
DECLINLINE(int) VbglR0SfHostReqCreate(SHFLROOT idRoot, VBOXSFCREATEREQ *pReq)
{
    uint32_t const cbReq = g_fHostFeatures & VMMDEV_HVF_HGCM_EMBEDDED_BUFFERS
                         ? RT_UOFFSETOF(VBOXSFCREATEREQ, StrPath.String) + pReq->StrPath.u16Size
                         : RT_UOFFSETOF(VBOXSFCREATEREQ, CreateParms);
    VBGLIOCIDCHGCMFASTCALL_INIT(&pReq->Hdr, VbglR0PhysHeapGetPhysAddr(pReq), &pReq->Call, g_SfClient.idClient,
                                SHFL_FN_CREATE, SHFL_CPARMS_CREATE, cbReq);

    pReq->Parms.id32Root.type                       = VMMDevHGCMParmType_32bit;
    pReq->Parms.id32Root.u.value32                  = idRoot;

    if (g_fHostFeatures & VMMDEV_HVF_HGCM_EMBEDDED_BUFFERS)
    {
        pReq->Parms.pStrPath.type                   = VMMDevHGCMParmType_Embedded;
        pReq->Parms.pStrPath.u.Embedded.cbData      = SHFLSTRING_HEADER_SIZE + pReq->StrPath.u16Size;
        pReq->Parms.pStrPath.u.Embedded.offData     = RT_UOFFSETOF(VBOXSFCREATEREQ, StrPath) - sizeof(VBGLIOCIDCHGCMFASTCALL);
        pReq->Parms.pStrPath.u.Embedded.fFlags      = VBOX_HGCM_F_PARM_DIRECTION_TO_HOST;

        pReq->Parms.pCreateParms.type               = VMMDevHGCMParmType_Embedded;
        pReq->Parms.pCreateParms.u.Embedded.cbData  = sizeof(pReq->CreateParms);
        pReq->Parms.pCreateParms.u.Embedded.offData = RT_UOFFSETOF(VBOXSFCREATEREQ, CreateParms) - sizeof(VBGLIOCIDCHGCMFASTCALL);
        pReq->Parms.pCreateParms.u.Embedded.fFlags  = VBOX_HGCM_F_PARM_DIRECTION_BOTH;
    }
    else
    {
        pReq->Parms.pStrPath.type                   = VMMDevHGCMParmType_LinAddr_In;
        pReq->Parms.pStrPath.u.LinAddr.cb           = SHFLSTRING_HEADER_SIZE + pReq->StrPath.u16Size;
        pReq->Parms.pStrPath.u.LinAddr.uAddr        = (uintptr_t)&pReq->StrPath;

        pReq->Parms.pCreateParms.type               = VMMDevHGCMParmType_LinAddr;
        pReq->Parms.pCreateParms.u.LinAddr.cb       = sizeof(pReq->CreateParms);
        pReq->Parms.pCreateParms.u.LinAddr.uAddr    = (uintptr_t)&pReq->CreateParms;
    }

    int vrc = VbglR0HGCMFastCall(g_SfClient.handle, &pReq->Hdr, cbReq);
    if (RT_SUCCESS(vrc))
        vrc = pReq->Call.header.result;
    return vrc;
}


/** Request structure for VbglR0SfHostReqClose.  */
typedef struct VBOXSFCLOSEREQ
{
    VBGLIOCIDCHGCMFASTCALL  Hdr;
    VMMDevHGCMCall          Call;
    VBoxSFParmClose         Parms;
} VBOXSFCLOSEREQ;

/**
 * SHFL_FN_CLOSE request.
 */
DECLINLINE(int) VbglR0SfHostReqClose(SHFLROOT idRoot, VBOXSFCLOSEREQ *pReq, uint64_t hHostFile)
{
    VBGLIOCIDCHGCMFASTCALL_INIT(&pReq->Hdr, VbglR0PhysHeapGetPhysAddr(pReq), &pReq->Call, g_SfClient.idClient,
                                SHFL_FN_CLOSE, SHFL_CPARMS_CLOSE, sizeof(*pReq));

    pReq->Parms.id32Root.type       = VMMDevHGCMParmType_32bit;
    pReq->Parms.id32Root.u.value32  = idRoot;

    pReq->Parms.u64Handle.type      = VMMDevHGCMParmType_64bit;
    pReq->Parms.u64Handle.u.value64 = hHostFile;

    int vrc = VbglR0HGCMFastCall(g_SfClient.handle, &pReq->Hdr, sizeof(*pReq));
    if (RT_SUCCESS(vrc))
        vrc = pReq->Call.header.result;
    return vrc;
}

/**
 * SHFL_FN_CLOSE request, allocate request buffer.
 */
DECLINLINE(int) VbglR0SfHostReqCloseSimple(SHFLROOT idRoot, uint64_t hHostFile)
{
    VBOXSFCLOSEREQ *pReq = (VBOXSFCLOSEREQ *)VbglR0PhysHeapAlloc(sizeof(*pReq));
    if (pReq)
    {
        int vrc = VbglR0SfHostReqClose(idRoot, pReq, hHostFile);
        VbglR0PhysHeapFree(pReq);
        return vrc;
    }
    return VERR_NO_MEMORY;
}


/** Request structure for VbglR0SfHostReqQueryVolInfo.  */
typedef struct VBOXSFVOLINFOREQ
{
    VBGLIOCIDCHGCMFASTCALL  Hdr;
    VMMDevHGCMCall          Call;
    VBoxSFParmInformation   Parms;
    SHFLVOLINFO             VolInfo;
} VBOXSFVOLINFOREQ;

/**
 * SHFL_FN_INFORMATION[SHFL_INFO_VOLUME | SHFL_INFO_GET] request.
 */
DECLINLINE(int) VbglR0SfHostReqQueryVolInfo(SHFLROOT idRoot, VBOXSFVOLINFOREQ *pReq, uint64_t hHostFile)
{
    uint32_t const cbReq = g_fHostFeatures & VMMDEV_HVF_HGCM_EMBEDDED_BUFFERS
                         ? sizeof(*pReq) : RT_UOFFSETOF(VBOXSFVOLINFOREQ, VolInfo);
    VBGLIOCIDCHGCMFASTCALL_INIT(&pReq->Hdr, VbglR0PhysHeapGetPhysAddr(pReq), &pReq->Call, g_SfClient.idClient,
                                SHFL_FN_INFORMATION, SHFL_CPARMS_INFORMATION, cbReq);

    pReq->Parms.id32Root.type                   = VMMDevHGCMParmType_32bit;
    pReq->Parms.id32Root.u.value32              = idRoot;

    pReq->Parms.u64Handle.type                  = VMMDevHGCMParmType_64bit;
    pReq->Parms.u64Handle.u.value64             = hHostFile;

    pReq->Parms.f32Flags.type                   = VMMDevHGCMParmType_32bit;
    pReq->Parms.f32Flags.u.value32              = SHFL_INFO_VOLUME | SHFL_INFO_GET;

    pReq->Parms.cb32.type                       = VMMDevHGCMParmType_32bit;
    pReq->Parms.cb32.u.value32                  = sizeof(pReq->VolInfo);

    if (g_fHostFeatures & VMMDEV_HVF_HGCM_EMBEDDED_BUFFERS)
    {
        pReq->Parms.pInfo.type                  = VMMDevHGCMParmType_Embedded;
        pReq->Parms.pInfo.u.Embedded.cbData     = sizeof(pReq->VolInfo);
        pReq->Parms.pInfo.u.Embedded.offData    = RT_UOFFSETOF(VBOXSFVOLINFOREQ, VolInfo) - sizeof(VBGLIOCIDCHGCMFASTCALL);
        pReq->Parms.pInfo.u.Embedded.fFlags     = VBOX_HGCM_F_PARM_DIRECTION_FROM_HOST;
    }
    else
    {
        pReq->Parms.pInfo.type                  = VMMDevHGCMParmType_LinAddr_Out;
        pReq->Parms.pInfo.u.LinAddr.cb          = sizeof(pReq->VolInfo);
        pReq->Parms.pInfo.u.LinAddr.uAddr       = (uintptr_t)&pReq->VolInfo;
    }

    int vrc = VbglR0HGCMFastCall(g_SfClient.handle, &pReq->Hdr, cbReq);
    if (RT_SUCCESS(vrc))
        vrc = pReq->Call.header.result;
    return vrc;
}


/** Request structure for VbglR0SfHostReqSetObjInfo & VbglR0SfHostReqQueryObjInfo. */
typedef struct VBOXSFOBJINFOREQ
{
    VBGLIOCIDCHGCMFASTCALL  Hdr;
    VMMDevHGCMCall          Call;
    VBoxSFParmInformation   Parms;
    SHFLFSOBJINFO           ObjInfo;
} VBOXSFOBJINFOREQ;

/**
 * SHFL_FN_INFORMATION[SHFL_INFO_GET | SHFL_INFO_FILE] request.
 */
DECLINLINE(int) VbglR0SfHostReqQueryObjInfo(SHFLROOT idRoot, VBOXSFOBJINFOREQ *pReq, uint64_t hHostFile)
{
    uint32_t const cbReq = g_fHostFeatures & VMMDEV_HVF_HGCM_EMBEDDED_BUFFERS
                         ? sizeof(*pReq) : RT_UOFFSETOF(VBOXSFOBJINFOREQ, ObjInfo);
    VBGLIOCIDCHGCMFASTCALL_INIT(&pReq->Hdr, VbglR0PhysHeapGetPhysAddr(pReq), &pReq->Call, g_SfClient.idClient,
                                SHFL_FN_INFORMATION, SHFL_CPARMS_INFORMATION, cbReq);

    pReq->Parms.id32Root.type                   = VMMDevHGCMParmType_32bit;
    pReq->Parms.id32Root.u.value32              = idRoot;

    pReq->Parms.u64Handle.type                  = VMMDevHGCMParmType_64bit;
    pReq->Parms.u64Handle.u.value64             = hHostFile;

    pReq->Parms.f32Flags.type                   = VMMDevHGCMParmType_32bit;
    pReq->Parms.f32Flags.u.value32              = SHFL_INFO_GET | SHFL_INFO_FILE;

    pReq->Parms.cb32.type                       = VMMDevHGCMParmType_32bit;
    pReq->Parms.cb32.u.value32                  = sizeof(pReq->ObjInfo);

    if (g_fHostFeatures & VMMDEV_HVF_HGCM_EMBEDDED_BUFFERS)
    {
        pReq->Parms.pInfo.type                  = VMMDevHGCMParmType_Embedded;
        pReq->Parms.pInfo.u.Embedded.cbData     = sizeof(pReq->ObjInfo);
        pReq->Parms.pInfo.u.Embedded.offData    = RT_UOFFSETOF(VBOXSFOBJINFOREQ, ObjInfo) - sizeof(VBGLIOCIDCHGCMFASTCALL);
        pReq->Parms.pInfo.u.Embedded.fFlags     = VBOX_HGCM_F_PARM_DIRECTION_FROM_HOST;
    }
    else
    {
        pReq->Parms.pInfo.type                  = VMMDevHGCMParmType_LinAddr_Out;
        pReq->Parms.pInfo.u.LinAddr.cb          = sizeof(pReq->ObjInfo);
        pReq->Parms.pInfo.u.LinAddr.uAddr       = (uintptr_t)&pReq->ObjInfo;
    }

    int vrc = VbglR0HGCMFastCall(g_SfClient.handle, &pReq->Hdr, cbReq);
    if (RT_SUCCESS(vrc))
        vrc = pReq->Call.header.result;
    return vrc;
}


/**
 * SHFL_FN_INFORMATION[SHFL_INFO_SET | SHFL_INFO_FILE] request.
 */
DECLINLINE(int) VbglR0SfHostReqSetObjInfo(SHFLROOT idRoot, VBOXSFOBJINFOREQ *pReq, uint64_t hHostFile)
{
    uint32_t const cbReq = g_fHostFeatures & VMMDEV_HVF_HGCM_EMBEDDED_BUFFERS
                         ? sizeof(*pReq) : RT_UOFFSETOF(VBOXSFOBJINFOREQ, ObjInfo);
    VBGLIOCIDCHGCMFASTCALL_INIT(&pReq->Hdr, VbglR0PhysHeapGetPhysAddr(pReq), &pReq->Call, g_SfClient.idClient,
                                SHFL_FN_INFORMATION, SHFL_CPARMS_INFORMATION, cbReq);

    pReq->Parms.id32Root.type                   = VMMDevHGCMParmType_32bit;
    pReq->Parms.id32Root.u.value32              = idRoot;

    pReq->Parms.u64Handle.type                  = VMMDevHGCMParmType_64bit;
    pReq->Parms.u64Handle.u.value64             = hHostFile;

    pReq->Parms.f32Flags.type                   = VMMDevHGCMParmType_32bit;
    pReq->Parms.f32Flags.u.value32              = SHFL_INFO_SET | SHFL_INFO_FILE;

    pReq->Parms.cb32.type                       = VMMDevHGCMParmType_32bit;
    pReq->Parms.cb32.u.value32                  = sizeof(pReq->ObjInfo);

    if (g_fHostFeatures & VMMDEV_HVF_HGCM_EMBEDDED_BUFFERS)
    {
        pReq->Parms.pInfo.type                  = VMMDevHGCMParmType_Embedded;
        pReq->Parms.pInfo.u.Embedded.cbData     = sizeof(pReq->ObjInfo);
        pReq->Parms.pInfo.u.Embedded.offData    = RT_UOFFSETOF(VBOXSFOBJINFOREQ, ObjInfo) - sizeof(VBGLIOCIDCHGCMFASTCALL);
        pReq->Parms.pInfo.u.Embedded.fFlags     = VBOX_HGCM_F_PARM_DIRECTION_BOTH;
    }
    else
    {
        pReq->Parms.pInfo.type                  = VMMDevHGCMParmType_LinAddr;
        pReq->Parms.pInfo.u.LinAddr.cb          = sizeof(pReq->ObjInfo);
        pReq->Parms.pInfo.u.LinAddr.uAddr       = (uintptr_t)&pReq->ObjInfo;
    }

    int vrc = VbglR0HGCMFastCall(g_SfClient.handle, &pReq->Hdr, cbReq);
    if (RT_SUCCESS(vrc))
        vrc = pReq->Call.header.result;
    return vrc;
}


/** Request structure for VbglR0SfHostReqSetObjInfo.  */
typedef struct VBOXSFOBJINFOWITHBUFREQ
{
    VBGLIOCIDCHGCMFASTCALL  Hdr;
    VMMDevHGCMCall          Call;
    VBoxSFParmInformation   Parms;
    HGCMPageListInfo        PgLst;
} VBOXSFOBJINFOWITHBUFREQ;

/**
 * SHFL_FN_INFORMATION[SHFL_INFO_SET | SHFL_INFO_FILE] request, with separate
 * buffer (on the physical heap).
 */
DECLINLINE(int) VbglR0SfHostReqSetObjInfoWithBuf(SHFLROOT idRoot, VBOXSFOBJINFOWITHBUFREQ *pReq, uint64_t hHostFile,
                                                 PSHFLFSOBJINFO pObjInfo, uint32_t offObjInfoInAlloc)
{
    VBGLIOCIDCHGCMFASTCALL_INIT(&pReq->Hdr, VbglR0PhysHeapGetPhysAddr(pReq), &pReq->Call, g_SfClient.idClient,
                                SHFL_FN_INFORMATION, SHFL_CPARMS_INFORMATION, sizeof(*pReq));

    pReq->Parms.id32Root.type               = VMMDevHGCMParmType_32bit;
    pReq->Parms.id32Root.u.value32          = idRoot;

    pReq->Parms.u64Handle.type              = VMMDevHGCMParmType_64bit;
    pReq->Parms.u64Handle.u.value64         = hHostFile;

    pReq->Parms.f32Flags.type               = VMMDevHGCMParmType_32bit;
    pReq->Parms.f32Flags.u.value32          = SHFL_INFO_SET | SHFL_INFO_FILE;

    pReq->Parms.cb32.type                   = VMMDevHGCMParmType_32bit;
    pReq->Parms.cb32.u.value32              = sizeof(*pObjInfo);

    if (g_fHostFeatures & VMMDEV_HVF_HGCM_CONTIGUOUS_PAGE_LIST)
    {
        pReq->Parms.pInfo.type              = VMMDevHGCMParmType_ContiguousPageList;
        pReq->Parms.pInfo.u.PageList.size   = sizeof(*pObjInfo);
        pReq->Parms.pInfo.u.PageList.offset = RT_UOFFSETOF(VBOXSFOBJINFOREQ, ObjInfo) - sizeof(VBGLIOCIDCHGCMFASTCALL);
        pReq->PgLst.flags                   = VBOX_HGCM_F_PARM_DIRECTION_BOTH;
        pReq->PgLst.aPages[0]               = VbglR0PhysHeapGetPhysAddr((uint8_t *)pObjInfo - offObjInfoInAlloc) + offObjInfoInAlloc;
        pReq->PgLst.offFirstPage            = (uint16_t)(pReq->PgLst.aPages[0] & PAGE_OFFSET_MASK);
        pReq->PgLst.aPages[0]              &= ~(RTGCPHYS)PAGE_OFFSET_MASK;
        pReq->PgLst.cPages                  = 1;
    }
    else
    {
        pReq->Parms.pInfo.type              = VMMDevHGCMParmType_LinAddr;
        pReq->Parms.pInfo.u.LinAddr.cb      = sizeof(*pObjInfo);
        pReq->Parms.pInfo.u.LinAddr.uAddr   = (uintptr_t)pObjInfo;
    }

    int vrc = VbglR0HGCMFastCall(g_SfClient.handle, &pReq->Hdr, sizeof(*pReq));
    if (RT_SUCCESS(vrc))
        vrc = pReq->Call.header.result;
    return vrc;
}


/** Request structure for VbglR0SfHostReqRemove.  */
typedef struct VBOXSFREMOVEREQ
{
    VBGLIOCIDCHGCMFASTCALL  Hdr;
    VMMDevHGCMCall          Call;
    VBoxSFParmRemove        Parms;
    SHFLSTRING              StrPath;
} VBOXSFREMOVEREQ;

/**
 * SHFL_FN_REMOVE request.
 */
DECLINLINE(int) VbglR0SfHostReqRemove(SHFLROOT idRoot, VBOXSFREMOVEREQ *pReq, uint32_t fFlags)
{
    uint32_t const cbReq = RT_UOFFSETOF(VBOXSFREMOVEREQ, StrPath.String)
                         + (g_fHostFeatures & VMMDEV_HVF_HGCM_EMBEDDED_BUFFERS ? pReq->StrPath.u16Size : 0);
    VBGLIOCIDCHGCMFASTCALL_INIT(&pReq->Hdr, VbglR0PhysHeapGetPhysAddr(pReq), &pReq->Call, g_SfClient.idClient,
                                SHFL_FN_REMOVE, SHFL_CPARMS_REMOVE, cbReq);

    pReq->Parms.id32Root.type                   = VMMDevHGCMParmType_32bit;
    pReq->Parms.id32Root.u.value32              = idRoot;

    if (g_fHostFeatures & VMMDEV_HVF_HGCM_EMBEDDED_BUFFERS)
    {
        pReq->Parms.pStrPath.type               = VMMDevHGCMParmType_Embedded;
        pReq->Parms.pStrPath.u.Embedded.cbData  = SHFLSTRING_HEADER_SIZE + pReq->StrPath.u16Size;
        pReq->Parms.pStrPath.u.Embedded.offData = RT_UOFFSETOF(VBOXSFREMOVEREQ, StrPath) - sizeof(VBGLIOCIDCHGCMFASTCALL);
        pReq->Parms.pStrPath.u.Embedded.fFlags  = VBOX_HGCM_F_PARM_DIRECTION_TO_HOST;
    }
    else
    {
        pReq->Parms.pStrPath.type               = VMMDevHGCMParmType_LinAddr_In;
        pReq->Parms.pStrPath.u.LinAddr.cb       = SHFLSTRING_HEADER_SIZE + pReq->StrPath.u16Size;
        pReq->Parms.pStrPath.u.LinAddr.uAddr    = (uintptr_t)&pReq->StrPath;
    }

    pReq->Parms.f32Flags.type                   = VMMDevHGCMParmType_32bit;
    pReq->Parms.f32Flags.u.value32              = fFlags;

    int vrc = VbglR0HGCMFastCall(g_SfClient.handle, &pReq->Hdr, cbReq);
    if (RT_SUCCESS(vrc))
        vrc = pReq->Call.header.result;
    return vrc;
}


/** Request structure for vboxSfOs2HostReqRename.  */
typedef struct VBOXSFRENAMEWITHSRCBUFREQ
{
    VBGLIOCIDCHGCMFASTCALL  Hdr;
    VMMDevHGCMCall          Call;
    VBoxSFParmRename        Parms;
    HGCMPageListInfo        PgLst;
    SHFLSTRING              StrDstPath;
} VBOXSFRENAMEWITHSRCBUFREQ;

/**
 * SHFL_FN_REMOVE request.
 */
DECLINLINE(int) VbglR0SfHostReqRenameWithSrcBuf(SHFLROOT idRoot, VBOXSFRENAMEWITHSRCBUFREQ *pReq,
                                                PSHFLSTRING pSrcStr, uint32_t fFlags)
{
    uint32_t const cbReq = RT_UOFFSETOF(VBOXSFRENAMEWITHSRCBUFREQ, StrDstPath.String)
                         + (g_fHostFeatures & VMMDEV_HVF_HGCM_EMBEDDED_BUFFERS ? pReq->StrDstPath.u16Size : 0);
    VBGLIOCIDCHGCMFASTCALL_INIT(&pReq->Hdr, VbglR0PhysHeapGetPhysAddr(pReq), &pReq->Call, g_SfClient.idClient,
                                SHFL_FN_RENAME, SHFL_CPARMS_RENAME, cbReq);

    pReq->Parms.id32Root.type                       = VMMDevHGCMParmType_32bit;
    pReq->Parms.id32Root.u.value32                  = idRoot;

    if (g_fHostFeatures & VMMDEV_HVF_HGCM_CONTIGUOUS_PAGE_LIST)
    {
        pReq->Parms.pStrSrcPath.type                = VMMDevHGCMParmType_ContiguousPageList;
        pReq->Parms.pStrSrcPath.u.PageList.size     = SHFLSTRING_HEADER_SIZE + pSrcStr->u16Size;
        pReq->Parms.pStrSrcPath.u.PageList.offset   = RT_UOFFSETOF(VBOXSFRENAMEWITHSRCBUFREQ, PgLst)
                                                    - sizeof(VBGLIOCIDCHGCMFASTCALL);
        pReq->PgLst.flags                           = VBOX_HGCM_F_PARM_DIRECTION_TO_HOST;
        pReq->PgLst.aPages[0]                       = VbglR0PhysHeapGetPhysAddr(pSrcStr);
        pReq->PgLst.offFirstPage                    = (uint16_t)(pReq->PgLst.aPages[0] & PAGE_OFFSET_MASK);
        pReq->PgLst.aPages[0]                      &= ~(RTGCPHYS)PAGE_OFFSET_MASK;
        pReq->PgLst.cPages                          = 1;
    }
    else
    {
        pReq->Parms.pStrSrcPath.type                = VMMDevHGCMParmType_LinAddr_In;
        pReq->Parms.pStrSrcPath.u.LinAddr.cb        = SHFLSTRING_HEADER_SIZE + pSrcStr->u16Size;
        pReq->Parms.pStrSrcPath.u.LinAddr.uAddr     = (uintptr_t)pSrcStr;
    }

    if (g_fHostFeatures & VMMDEV_HVF_HGCM_EMBEDDED_BUFFERS)
    {
        pReq->Parms.pStrDstPath.type                = VMMDevHGCMParmType_Embedded;
        pReq->Parms.pStrDstPath.u.Embedded.cbData   = SHFLSTRING_HEADER_SIZE + pReq->StrDstPath.u16Size;
        pReq->Parms.pStrDstPath.u.Embedded.offData  = RT_UOFFSETOF(VBOXSFRENAMEWITHSRCBUFREQ, StrDstPath)
                                                    - sizeof(VBGLIOCIDCHGCMFASTCALL);
        pReq->Parms.pStrDstPath.u.Embedded.fFlags   = VBOX_HGCM_F_PARM_DIRECTION_TO_HOST;
    }
    else
    {
        pReq->Parms.pStrDstPath.type                = VMMDevHGCMParmType_LinAddr_In;
        pReq->Parms.pStrDstPath.u.LinAddr.cb        = SHFLSTRING_HEADER_SIZE + pReq->StrDstPath.u16Size;
        pReq->Parms.pStrDstPath.u.LinAddr.uAddr     = (uintptr_t)&pReq->StrDstPath;
    }

    pReq->Parms.f32Flags.type                       = VMMDevHGCMParmType_32bit;
    pReq->Parms.f32Flags.u.value32                  = fFlags;

    int vrc = VbglR0HGCMFastCall(g_SfClient.handle, &pReq->Hdr, cbReq);
    if (RT_SUCCESS(vrc))
        vrc = pReq->Call.header.result;
    return vrc;
}


/** Request structure for VbglR0SfHostReqFlush.  */
typedef struct VBOXSFFLUSHREQ
{
    VBGLIOCIDCHGCMFASTCALL  Hdr;
    VMMDevHGCMCall          Call;
    VBoxSFParmFlush         Parms;
} VBOXSFFLUSHREQ;

/**
 * SHFL_FN_FLUSH request.
 */
DECLINLINE(int) VbglR0SfHostReqFlush(SHFLROOT idRoot, VBOXSFFLUSHREQ *pReq, uint64_t hHostFile)
{
    VBGLIOCIDCHGCMFASTCALL_INIT(&pReq->Hdr, VbglR0PhysHeapGetPhysAddr(pReq), &pReq->Call, g_SfClient.idClient,
                                SHFL_FN_FLUSH, SHFL_CPARMS_FLUSH, sizeof(*pReq));

    pReq->Parms.id32Root.type       = VMMDevHGCMParmType_32bit;
    pReq->Parms.id32Root.u.value32  = idRoot;

    pReq->Parms.u64Handle.type      = VMMDevHGCMParmType_64bit;
    pReq->Parms.u64Handle.u.value64 = hHostFile;

    int vrc = VbglR0HGCMFastCall(g_SfClient.handle, &pReq->Hdr, sizeof(*pReq));
    if (RT_SUCCESS(vrc))
        vrc = pReq->Call.header.result;
    return vrc;
}

/**
 * SHFL_FN_FLUSH request, allocate request buffer.
 */
DECLINLINE(int) VbglR0SfHostReqFlushSimple(SHFLROOT idRoot, uint64_t hHostFile)
{
    VBOXSFFLUSHREQ *pReq = (VBOXSFFLUSHREQ *)VbglR0PhysHeapAlloc(sizeof(*pReq));
    if (pReq)
    {
        int vrc = VbglR0SfHostReqFlush(idRoot, pReq, hHostFile);
        VbglR0PhysHeapFree(pReq);
        return vrc;
    }
    return VERR_NO_MEMORY;
}


/** Request structure for VbglR0SfHostReqSetFileSize.  */
typedef struct VBOXSFSETFILESIZEREQ
{
    VBGLIOCIDCHGCMFASTCALL  Hdr;
    VMMDevHGCMCall          Call;
    VBoxSFParmSetFileSize   Parms;
} VBOXSFSETFILESIZEREQ;

/**
 * SHFL_FN_SET_FILE_SIZE request.
 */
DECLINLINE(int) VbglR0SfHostReqSetFileSize(SHFLROOT idRoot, VBOXSFSETFILESIZEREQ *pReq, uint64_t hHostFile, uint64_t cbNewSize)
{
    VBGLIOCIDCHGCMFASTCALL_INIT(&pReq->Hdr, VbglR0PhysHeapGetPhysAddr(pReq), &pReq->Call, g_SfClient.idClient,
                                SHFL_FN_SET_FILE_SIZE, SHFL_CPARMS_SET_FILE_SIZE, sizeof(*pReq));

    pReq->Parms.id32Root.type           = VMMDevHGCMParmType_32bit;
    pReq->Parms.id32Root.u.value32      = idRoot;

    pReq->Parms.u64Handle.type          = VMMDevHGCMParmType_64bit;
    pReq->Parms.u64Handle.u.value64     = hHostFile;

    pReq->Parms.cb64NewSize.type        = VMMDevHGCMParmType_64bit;
    pReq->Parms.cb64NewSize.u.value64   = cbNewSize;

    int vrc = VbglR0HGCMFastCall(g_SfClient.handle, &pReq->Hdr, sizeof(*pReq));
    if (RT_SUCCESS(vrc))
        vrc = pReq->Call.header.result;
    return vrc;
}

/**
 * SHFL_FN_SET_FILE_SIZE request, allocate request buffer.
 */
DECLINLINE(int) VbglR0SfHostReqSetFileSizeSimple(SHFLROOT idRoot, uint64_t hHostFile, uint64_t cbNewSize)
{
    VBOXSFSETFILESIZEREQ *pReq = (VBOXSFSETFILESIZEREQ *)VbglR0PhysHeapAlloc(sizeof(*pReq));
    if (pReq)
    {
        int vrc = VbglR0SfHostReqSetFileSize(idRoot, pReq, hHostFile, cbNewSize);
        VbglR0PhysHeapFree(pReq);
        return vrc;
    }
    return VERR_NO_MEMORY;
}


/** Request structure for VbglR0SfHostReqReadEmbedded. */
typedef struct VBOXSFREADEMBEDDEDREQ
{
    VBGLIOCIDCHGCMFASTCALL  Hdr;
    VMMDevHGCMCall          Call;
    VBoxSFParmRead          Parms;
    uint8_t                 abData[RT_FLEXIBLE_ARRAY];
} VBOXSFREADEMBEDDEDREQ;

/**
 * SHFL_FN_READ request using embedded data buffer.
 */
DECLINLINE(int) VbglR0SfHostReqReadEmbedded(SHFLROOT idRoot, VBOXSFREADEMBEDDEDREQ *pReq, uint64_t hHostFile,
                                            uint64_t offRead, uint32_t cbToRead)
{
    uint32_t const cbReq = RT_UOFFSETOF(VBOXSFREADEMBEDDEDREQ, abData[0])
                         + (g_fHostFeatures & VMMDEV_HVF_HGCM_EMBEDDED_BUFFERS ? cbToRead : 0);
    VBGLIOCIDCHGCMFASTCALL_INIT(&pReq->Hdr, VbglR0PhysHeapGetPhysAddr(pReq), &pReq->Call, g_SfClient.idClient,
                                SHFL_FN_READ, SHFL_CPARMS_READ, cbReq);

    pReq->Parms.id32Root.type               = VMMDevHGCMParmType_32bit;
    pReq->Parms.id32Root.u.value32          = idRoot;

    pReq->Parms.u64Handle.type              = VMMDevHGCMParmType_64bit;
    pReq->Parms.u64Handle.u.value64         = hHostFile;

    pReq->Parms.off64Read.type              = VMMDevHGCMParmType_64bit;
    pReq->Parms.off64Read.u.value64         = offRead;

    pReq->Parms.cb32Read.type               = VMMDevHGCMParmType_32bit;
    pReq->Parms.cb32Read.u.value32          = cbToRead;

    if (g_fHostFeatures & VMMDEV_HVF_HGCM_EMBEDDED_BUFFERS)
    {
        pReq->Parms.pBuf.type               = VMMDevHGCMParmType_Embedded;
        pReq->Parms.pBuf.u.Embedded.cbData  = cbToRead;
        pReq->Parms.pBuf.u.Embedded.offData = RT_UOFFSETOF(VBOXSFREADEMBEDDEDREQ, abData[0]) - sizeof(VBGLIOCIDCHGCMFASTCALL);
        pReq->Parms.pBuf.u.Embedded.fFlags  = VBOX_HGCM_F_PARM_DIRECTION_FROM_HOST;
    }
    else
    {
        pReq->Parms.pBuf.type               = VMMDevHGCMParmType_LinAddr_Out;
        pReq->Parms.pBuf.u.LinAddr.cb       = cbToRead;
        pReq->Parms.pBuf.u.LinAddr.uAddr    = (uintptr_t)&pReq->abData[0];
    }

    int vrc = VbglR0HGCMFastCall(g_SfClient.handle, &pReq->Hdr, cbReq);
    if (RT_SUCCESS(vrc))
        vrc = pReq->Call.header.result;
    return vrc;
}


/** Request structure for vboxSfOs2HostReqRead & VbglR0SfHostReqReadContig. */
typedef struct VBOXSFREADPGLSTREQ
{
    VBGLIOCIDCHGCMFASTCALL  Hdr;
    VMMDevHGCMCall          Call;
    VBoxSFParmRead          Parms;
    HGCMPageListInfo        PgLst;
} VBOXSFREADPGLSTREQ;

/**
 * SHFL_FN_READ request using page list for data buffer (caller populated).
 */
DECLINLINE(int) VbglR0SfHostReqReadPgLst(SHFLROOT idRoot, VBOXSFREADPGLSTREQ *pReq, uint64_t hHostFile,
                                         uint64_t offRead, uint32_t cbToRead, uint32_t cPages)
{
    VBGLIOCIDCHGCMFASTCALL_INIT(&pReq->Hdr, VbglR0PhysHeapGetPhysAddr(pReq), &pReq->Call, g_SfClient.idClient,
                                SHFL_FN_READ, SHFL_CPARMS_READ,
                                RT_UOFFSETOF_DYN(VBOXSFREADPGLSTREQ, PgLst.aPages[cPages]));

    pReq->Parms.id32Root.type               = VMMDevHGCMParmType_32bit;
    pReq->Parms.id32Root.u.value32          = idRoot;

    pReq->Parms.u64Handle.type              = VMMDevHGCMParmType_64bit;
    pReq->Parms.u64Handle.u.value64         = hHostFile;

    pReq->Parms.off64Read.type              = VMMDevHGCMParmType_64bit;
    pReq->Parms.off64Read.u.value64         = offRead;

    pReq->Parms.cb32Read.type               = VMMDevHGCMParmType_32bit;
    pReq->Parms.cb32Read.u.value32          = cbToRead;

    pReq->Parms.pBuf.type                   = VMMDevHGCMParmType_PageList;
    pReq->Parms.pBuf.u.PageList.size        = cbToRead;
    pReq->Parms.pBuf.u.PageList.offset      = RT_UOFFSETOF(VBOXSFREADPGLSTREQ, PgLst) - sizeof(VBGLIOCIDCHGCMFASTCALL);
    pReq->PgLst.flags                       = VBOX_HGCM_F_PARM_DIRECTION_FROM_HOST;
    pReq->PgLst.cPages                      = (uint16_t)cPages;
    AssertReturn(cPages <= UINT16_MAX, VERR_OUT_OF_RANGE);
    /* caller sets offset */

    int vrc = VbglR0HGCMFastCall(g_SfClient.handle, &pReq->Hdr,
                                 RT_UOFFSETOF_DYN(VBOXSFREADPGLSTREQ, PgLst.aPages[cPages]));
    if (RT_SUCCESS(vrc))
        vrc = pReq->Call.header.result;
    return vrc;
}


/**
 * SHFL_FN_READ request using a physically contiguous buffer.
 */
DECLINLINE(int) VbglR0SfHostReqReadContig(SHFLROOT idRoot, VBOXSFREADPGLSTREQ *pReq, uint64_t hHostFile,
                                          uint64_t offRead, uint32_t cbToRead, void *pvBuffer, RTGCPHYS64 PhysBuffer)
{
    VBGLIOCIDCHGCMFASTCALL_INIT(&pReq->Hdr, VbglR0PhysHeapGetPhysAddr(pReq), &pReq->Call, g_SfClient.idClient,
                                SHFL_FN_READ, SHFL_CPARMS_READ, RT_UOFFSETOF_DYN(VBOXSFREADPGLSTREQ, PgLst.aPages[1]));

    pReq->Parms.id32Root.type               = VMMDevHGCMParmType_32bit;
    pReq->Parms.id32Root.u.value32          = idRoot;

    pReq->Parms.u64Handle.type              = VMMDevHGCMParmType_64bit;
    pReq->Parms.u64Handle.u.value64         = hHostFile;

    pReq->Parms.off64Read.type              = VMMDevHGCMParmType_64bit;
    pReq->Parms.off64Read.u.value64         = offRead;

    pReq->Parms.cb32Read.type               = VMMDevHGCMParmType_32bit;
    pReq->Parms.cb32Read.u.value32          = cbToRead;

    if (g_fHostFeatures & VMMDEV_HVF_HGCM_CONTIGUOUS_PAGE_LIST)
    {
        pReq->Parms.pBuf.type               = VMMDevHGCMParmType_ContiguousPageList;
        pReq->Parms.pBuf.u.PageList.size    = cbToRead;
        pReq->Parms.pBuf.u.PageList.offset  = RT_UOFFSETOF(VBOXSFREADPGLSTREQ, PgLst) - sizeof(VBGLIOCIDCHGCMFASTCALL);
        pReq->PgLst.flags                   = VBOX_HGCM_F_PARM_DIRECTION_FROM_HOST;
        pReq->PgLst.offFirstPage            = (uint16_t)(PhysBuffer & PAGE_OFFSET_MASK);
        pReq->PgLst.cPages                  = 1;
        pReq->PgLst.aPages[0]               = PhysBuffer & ~(RTGCPHYS64)PAGE_OFFSET_MASK;
    }
    else
    {
        pReq->Parms.pBuf.type               = VMMDevHGCMParmType_LinAddr_Out;
        pReq->Parms.pBuf.u.LinAddr.cb       = cbToRead;
        pReq->Parms.pBuf.u.LinAddr.uAddr    = (uintptr_t)pvBuffer;
    }

    int vrc = VbglR0HGCMFastCall(g_SfClient.handle, &pReq->Hdr, RT_UOFFSETOF_DYN(VBOXSFREADPGLSTREQ, PgLst.aPages[1]));
    if (RT_SUCCESS(vrc))
        vrc = pReq->Call.header.result;
    return vrc;
}



/** Request structure for VbglR0SfHostReqWriteEmbedded. */
typedef struct VBOXSFWRITEEMBEDDEDREQ
{
    VBGLIOCIDCHGCMFASTCALL  Hdr;
    VMMDevHGCMCall          Call;
    VBoxSFParmWrite         Parms;
    uint8_t                 abData[RT_FLEXIBLE_ARRAY];
} VBOXSFWRITEEMBEDDEDREQ;

/**
 * SHFL_FN_WRITE request using embedded data buffer.
 */
DECLINLINE(int) VbglR0SfHostReqWriteEmbedded(SHFLROOT idRoot, VBOXSFWRITEEMBEDDEDREQ *pReq, uint64_t hHostFile,
                                             uint64_t offWrite, uint32_t cbToWrite)
{
    uint32_t const cbReq = RT_UOFFSETOF(VBOXSFWRITEEMBEDDEDREQ, abData[0])
                         + (g_fHostFeatures & VMMDEV_HVF_HGCM_EMBEDDED_BUFFERS ? cbToWrite : 0);
    VBGLIOCIDCHGCMFASTCALL_INIT(&pReq->Hdr, VbglR0PhysHeapGetPhysAddr(pReq), &pReq->Call, g_SfClient.idClient,
                                SHFL_FN_WRITE, SHFL_CPARMS_WRITE, cbReq);

    pReq->Parms.id32Root.type               = VMMDevHGCMParmType_32bit;
    pReq->Parms.id32Root.u.value32          = idRoot;

    pReq->Parms.u64Handle.type              = VMMDevHGCMParmType_64bit;
    pReq->Parms.u64Handle.u.value64         = hHostFile;

    pReq->Parms.off64Write.type             = VMMDevHGCMParmType_64bit;
    pReq->Parms.off64Write.u.value64        = offWrite;

    pReq->Parms.cb32Write.type              = VMMDevHGCMParmType_32bit;
    pReq->Parms.cb32Write.u.value32         = cbToWrite;

    if (g_fHostFeatures & VMMDEV_HVF_HGCM_EMBEDDED_BUFFERS)
    {
        pReq->Parms.pBuf.type               = VMMDevHGCMParmType_Embedded;
        pReq->Parms.pBuf.u.Embedded.cbData  = cbToWrite;
        pReq->Parms.pBuf.u.Embedded.offData = RT_UOFFSETOF(VBOXSFWRITEEMBEDDEDREQ, abData[0]) - sizeof(VBGLIOCIDCHGCMFASTCALL);
        pReq->Parms.pBuf.u.Embedded.fFlags  = VBOX_HGCM_F_PARM_DIRECTION_TO_HOST;
    }
    else
    {
        pReq->Parms.pBuf.type               = VMMDevHGCMParmType_LinAddr_In;
        pReq->Parms.pBuf.u.LinAddr.cb       = cbToWrite;
        pReq->Parms.pBuf.u.LinAddr.uAddr    = (uintptr_t)&pReq->abData[0];
    }

    int vrc = VbglR0HGCMFastCall(g_SfClient.handle, &pReq->Hdr, cbReq);
    if (RT_SUCCESS(vrc))
        vrc = pReq->Call.header.result;
    return vrc;
}


/** Request structure for vboxSfOs2HostReqWrite and VbglR0SfHostReqWriteContig. */
typedef struct VBOXSFWRITEPGLSTREQ
{
    VBGLIOCIDCHGCMFASTCALL  Hdr;
    VMMDevHGCMCall          Call;
    VBoxSFParmWrite         Parms;
    HGCMPageListInfo        PgLst;
} VBOXSFWRITEPGLSTREQ;

/**
 * SHFL_FN_WRITE request using page list for data buffer (caller populated).
 */
DECLINLINE(int) VbglR0SfHostReqWritePgLst(SHFLROOT idRoot, VBOXSFWRITEPGLSTREQ *pReq, uint64_t hHostFile,
                                          uint64_t offWrite, uint32_t cbToWrite, uint32_t cPages)
{
    VBGLIOCIDCHGCMFASTCALL_INIT(&pReq->Hdr, VbglR0PhysHeapGetPhysAddr(pReq), &pReq->Call, g_SfClient.idClient,
                                SHFL_FN_WRITE, SHFL_CPARMS_WRITE,
                                RT_UOFFSETOF_DYN(VBOXSFWRITEPGLSTREQ, PgLst.aPages[cPages]));

    pReq->Parms.id32Root.type               = VMMDevHGCMParmType_32bit;
    pReq->Parms.id32Root.u.value32          = idRoot;

    pReq->Parms.u64Handle.type              = VMMDevHGCMParmType_64bit;
    pReq->Parms.u64Handle.u.value64         = hHostFile;

    pReq->Parms.off64Write.type             = VMMDevHGCMParmType_64bit;
    pReq->Parms.off64Write.u.value64        = offWrite;

    pReq->Parms.cb32Write.type              = VMMDevHGCMParmType_32bit;
    pReq->Parms.cb32Write.u.value32         = cbToWrite;

    pReq->Parms.pBuf.type                   = VMMDevHGCMParmType_PageList;
    pReq->Parms.pBuf.u.PageList.size        = cbToWrite;
    pReq->Parms.pBuf.u.PageList.offset      = RT_UOFFSETOF(VBOXSFWRITEPGLSTREQ, PgLst) - sizeof(VBGLIOCIDCHGCMFASTCALL);
    pReq->PgLst.flags                       = VBOX_HGCM_F_PARM_DIRECTION_TO_HOST;
    pReq->PgLst.cPages                      = (uint16_t)cPages;
    AssertReturn(cPages <= UINT16_MAX, VERR_OUT_OF_RANGE);
    /* caller sets offset */

    int vrc = VbglR0HGCMFastCall(g_SfClient.handle, &pReq->Hdr,
                                 RT_UOFFSETOF_DYN(VBOXSFWRITEPGLSTREQ, PgLst.aPages[cPages]));
    if (RT_SUCCESS(vrc))
        vrc = pReq->Call.header.result;
    return vrc;
}


/**
 * SHFL_FN_WRITE request using a physically contiguous buffer.
 */
DECLINLINE(int) VbglR0SfHostReqWriteContig(SHFLROOT idRoot, VBOXSFWRITEPGLSTREQ *pReq, uint64_t hHostFile,
                                           uint64_t offWrite, uint32_t cbToWrite, void const *pvBuffer, RTGCPHYS64 PhysBuffer)
{
    VBGLIOCIDCHGCMFASTCALL_INIT(&pReq->Hdr, VbglR0PhysHeapGetPhysAddr(pReq), &pReq->Call, g_SfClient.idClient,
                                SHFL_FN_WRITE, SHFL_CPARMS_WRITE, RT_UOFFSETOF_DYN(VBOXSFWRITEPGLSTREQ, PgLst.aPages[1]));

    pReq->Parms.id32Root.type               = VMMDevHGCMParmType_32bit;
    pReq->Parms.id32Root.u.value32          = idRoot;

    pReq->Parms.u64Handle.type              = VMMDevHGCMParmType_64bit;
    pReq->Parms.u64Handle.u.value64         = hHostFile;

    pReq->Parms.off64Write.type             = VMMDevHGCMParmType_64bit;
    pReq->Parms.off64Write.u.value64        = offWrite;

    pReq->Parms.cb32Write.type              = VMMDevHGCMParmType_32bit;
    pReq->Parms.cb32Write.u.value32         = cbToWrite;

    if (g_fHostFeatures & VMMDEV_HVF_HGCM_CONTIGUOUS_PAGE_LIST)
    {
        pReq->Parms.pBuf.type               = VMMDevHGCMParmType_ContiguousPageList;
        pReq->Parms.pBuf.u.PageList.size    = cbToWrite;
        pReq->Parms.pBuf.u.PageList.offset  = RT_UOFFSETOF(VBOXSFWRITEPGLSTREQ, PgLst) - sizeof(VBGLIOCIDCHGCMFASTCALL);
        pReq->PgLst.flags                   = VBOX_HGCM_F_PARM_DIRECTION_TO_HOST;
        pReq->PgLst.offFirstPage            = (uint16_t)(PhysBuffer & PAGE_OFFSET_MASK);
        pReq->PgLst.cPages                  = 1;
        pReq->PgLst.aPages[0]               = PhysBuffer & ~(RTGCPHYS64)PAGE_OFFSET_MASK;
    }
    else
    {
        pReq->Parms.pBuf.type               = VMMDevHGCMParmType_LinAddr_In;
        pReq->Parms.pBuf.u.LinAddr.cb       = cbToWrite;
        pReq->Parms.pBuf.u.LinAddr.uAddr    = (uintptr_t)pvBuffer;
    }

    int vrc = VbglR0HGCMFastCall(g_SfClient.handle, &pReq->Hdr, RT_UOFFSETOF_DYN(VBOXSFWRITEPGLSTREQ, PgLst.aPages[1]));
    if (RT_SUCCESS(vrc))
        vrc = pReq->Call.header.result;
    return vrc;
}

/** Request structure for vboxSfOs2HostReqListDir. */
typedef struct VBOXSFLISTDIRREQ
{
    VBGLIOCIDCHGCMFASTCALL  Hdr;
    VMMDevHGCMCall          Call;
    VBoxSFParmList          Parms;
    HGCMPageListInfo        StrPgLst;
    HGCMPageListInfo        BufPgLst;
} VBOXSFLISTDIRREQ;

/**
 * SHFL_FN_LIST request with separate string buffer and buffers for entries,
 * both allocated on the physical heap.
 */
DECLINLINE(int) VbglR0SfHostReqListDir(SHFLROOT idRoot, VBOXSFLISTDIRREQ *pReq, uint64_t hHostDir,
                                       PSHFLSTRING pFilter, uint32_t fFlags, PSHFLDIRINFO pBuffer, uint32_t cbBuffer)
{
    VBGLIOCIDCHGCMFASTCALL_INIT(&pReq->Hdr, VbglR0PhysHeapGetPhysAddr(pReq), &pReq->Call, g_SfClient.idClient,
                                SHFL_FN_LIST, SHFL_CPARMS_LIST, sizeof(*pReq));

    pReq->Parms.id32Root.type                       = VMMDevHGCMParmType_32bit;
    pReq->Parms.id32Root.u.value32                  = idRoot;

    pReq->Parms.u64Handle.type                      = VMMDevHGCMParmType_64bit;
    pReq->Parms.u64Handle.u.value64                 = hHostDir;

    pReq->Parms.f32Flags.type                       = VMMDevHGCMParmType_32bit;
    pReq->Parms.f32Flags.u.value32                  = fFlags;

    pReq->Parms.cb32Buffer.type                     = VMMDevHGCMParmType_32bit;
    pReq->Parms.cb32Buffer.u.value32                = cbBuffer;

    if (g_fHostFeatures & VMMDEV_HVF_HGCM_CONTIGUOUS_PAGE_LIST)
    {
        pReq->Parms.pStrFilter.type                 = VMMDevHGCMParmType_ContiguousPageList;
        pReq->Parms.pStrFilter.u.PageList.offset    = RT_UOFFSETOF(VBOXSFLISTDIRREQ, StrPgLst) - sizeof(VBGLIOCIDCHGCMFASTCALL);
        pReq->StrPgLst.flags                        = VBOX_HGCM_F_PARM_DIRECTION_TO_HOST;
        pReq->StrPgLst.cPages                       = 1;
        if (pFilter)
        {
            pReq->Parms.pStrFilter.u.PageList.size  = SHFLSTRING_HEADER_SIZE + pFilter->u16Size;
            RTGCPHYS32 const GCPhys       = VbglR0PhysHeapGetPhysAddr(pFilter);
            uint32_t const   offFirstPage = GCPhys & PAGE_OFFSET_MASK;
            pReq->StrPgLst.offFirstPage             = (uint16_t)offFirstPage;
            pReq->StrPgLst.aPages[0]                = GCPhys - offFirstPage;
        }
        else
        {
            pReq->Parms.pStrFilter.u.PageList.size  = 0;
            pReq->StrPgLst.offFirstPage             = 0;
            pReq->StrPgLst.aPages[0]                = NIL_RTGCPHYS64;
        }

        pReq->Parms.pBuffer.type                    = VMMDevHGCMParmType_ContiguousPageList;
        pReq->Parms.pBuffer.u.PageList.offset       = RT_UOFFSETOF(VBOXSFLISTDIRREQ, BufPgLst) - sizeof(VBGLIOCIDCHGCMFASTCALL);
        pReq->Parms.pBuffer.u.PageList.size         = cbBuffer;
        pReq->BufPgLst.flags                        = VBOX_HGCM_F_PARM_DIRECTION_FROM_HOST;
        pReq->BufPgLst.cPages                       = 1;
        RTGCPHYS32 const GCPhys       = VbglR0PhysHeapGetPhysAddr(pBuffer);
        uint32_t const   offFirstPage = GCPhys & PAGE_OFFSET_MASK;
        pReq->BufPgLst.offFirstPage                 = (uint16_t)offFirstPage;
        pReq->BufPgLst.aPages[0]                    = GCPhys - offFirstPage;
    }
    else
    {
        pReq->Parms.pStrFilter.type                 = VMMDevHGCMParmType_LinAddr_In;
        pReq->Parms.pStrFilter.u.LinAddr.cb         = pFilter ? SHFLSTRING_HEADER_SIZE + pFilter->u16Size : 0;
        pReq->Parms.pStrFilter.u.LinAddr.uAddr      = (uintptr_t)pFilter;

        pReq->Parms.pBuffer.type                    = VMMDevHGCMParmType_LinAddr_Out;
        pReq->Parms.pBuffer.u.LinAddr.cb            = cbBuffer;
        pReq->Parms.pBuffer.u.LinAddr.uAddr         = (uintptr_t)pBuffer;
    }

    pReq->Parms.f32Done.type                        = VMMDevHGCMParmType_32bit;
    pReq->Parms.f32Done.u.value32                   = 0;

    pReq->Parms.c32Entries.type                     = VMMDevHGCMParmType_32bit;
    pReq->Parms.c32Entries.u.value32                = 0;

    int vrc = VbglR0HGCMFastCall(g_SfClient.handle, &pReq->Hdr, sizeof(*pReq));
    if (RT_SUCCESS(vrc))
        vrc = pReq->Call.header.result;
    return vrc;
}

/** @} */

#endif /* !VBOX_INCLUDED_VBoxGuestLibSharedFoldersInline_h */

