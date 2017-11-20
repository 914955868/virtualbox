#include "internal/path.h"
static int rtDirRelBuildFullPath(PRTDIRINTERNAL pThis, char *pszPathDst, size_t cbPathDst, const char *pszRelPath)
RTDECL(int)  RTDirRelFileOpen(RTDIR hDir, const char *pszRelFilename, uint64_t fOpen, PRTFILE phFile)
    PRTDIRINTERNAL pThis = hDir;
        rc = RTNtPathRelativeFromUtf8(&NtName, &hRoot, pszRelFilename, RTDIRREL_NT_GET_ASCENT(pThis),
                                      pThis->enmInfoClass == FileMaximumInformation);
RTDECL(int) RTDirRelDirOpen(RTDIR hDir, const char *pszDir, RTDIR *phDir)
RTDECL(int) RTDirRelDirOpenFiltered(RTDIR hDir, const char *pszDirAndFilter, RTDIRFILTER enmFilter,
                                    uint32_t fFlags, RTDIR *phDir)
    PRTDIRINTERNAL pThis = hDir;
    int rc = RTNtPathRelativeFromUtf8(&NtName, &hRoot, pszDirAndFilter, RTDIRREL_NT_GET_ASCENT(pThis),
                                      pThis->enmInfoClass == FileMaximumInformation);
        rc = rtDirOpenRelativeOrHandle(phDir, pszDirAndFilter, enmFilter, fFlags, (uintptr_t)hRoot, &NtName);
RTDECL(int) RTDirRelDirCreate(RTDIR hDir, const char *pszRelPath, RTFMODE fMode, uint32_t fCreate, RTDIR *phSubDir)
    PRTDIRINTERNAL pThis = hDir;
    AssertReturn(!(fCreate & ~RTDIRCREATE_FLAGS_VALID_MASK), VERR_INVALID_FLAGS);
    fMode = rtFsModeNormalize(fMode, pszRelPath, 0);
    AssertReturn(rtFsModeIsValidPermissions(fMode), VERR_INVALID_FMODE);
    AssertPtrNullReturn(phSubDir, VERR_INVALID_POINTER);
    /*
     * Convert and normalize the path.
     */
    UNICODE_STRING NtName;
    HANDLE hRoot = pThis->hDir;
    int rc = RTNtPathRelativeFromUtf8(&NtName, &hRoot, pszRelPath, RTDIRREL_NT_GET_ASCENT(pThis),
                                      pThis->enmInfoClass == FileMaximumInformation);
    {
        HANDLE              hNewDir = RTNT_INVALID_HANDLE_VALUE;
        IO_STATUS_BLOCK     Ios     = RTNT_IO_STATUS_BLOCK_INITIALIZER;
        OBJECT_ATTRIBUTES   ObjAttr;
        InitializeObjectAttributes(&ObjAttr, &NtName, 0 /*fAttrib*/, hRoot, NULL);

        ULONG fDirAttribs = (fCreate & RTFS_DOS_MASK_NT) >> RTFS_DOS_SHIFT;
        if (!(fCreate & RTDIRCREATE_FLAGS_NOT_CONTENT_INDEXED_DONT_SET))
            fDirAttribs |= FILE_ATTRIBUTE_NOT_CONTENT_INDEXED;
        if (!fDirAttribs)
            fDirAttribs = FILE_ATTRIBUTE_NORMAL;

        NTSTATUS rcNt = NtCreateFile(&hNewDir,
                                     phSubDir
                                     ? FILE_WRITE_ATTRIBUTES | FILE_READ_ATTRIBUTES | FILE_LIST_DIRECTORY | FILE_TRAVERSE | SYNCHRONIZE
                                     : SYNCHRONIZE,
                                     &ObjAttr,
                                     &Ios,
                                     NULL /*AllocationSize*/,
                                     fDirAttribs,
                                     FILE_SHARE_READ | FILE_SHARE_WRITE,
                                     FILE_CREATE,
                                     FILE_DIRECTORY_FILE | FILE_OPEN_FOR_BACKUP_INTENT | FILE_SYNCHRONOUS_IO_NONALERT,
                                     NULL /*EaBuffer*/,
                                     0 /*EaLength*/);

        /* Just in case someone takes offence at FILE_ATTRIBUTE_NOT_CONTENT_INDEXED. */
        if (   (   rcNt == STATUS_INVALID_PARAMETER
                || rcNt == STATUS_INVALID_PARAMETER_7)
            && (fDirAttribs & FILE_ATTRIBUTE_NOT_CONTENT_INDEXED)
            && (fCreate & RTDIRCREATE_FLAGS_NOT_CONTENT_INDEXED_NOT_CRITICAL) )
        {
            fDirAttribs &= ~FILE_ATTRIBUTE_NOT_CONTENT_INDEXED;
            if (!fDirAttribs)
                fDirAttribs = FILE_ATTRIBUTE_NORMAL;
            rcNt = NtCreateFile(&hNewDir,
                                phSubDir
                                ? FILE_WRITE_ATTRIBUTES | FILE_READ_ATTRIBUTES | FILE_LIST_DIRECTORY | FILE_TRAVERSE | SYNCHRONIZE
                                : SYNCHRONIZE,
                                &ObjAttr,
                                &Ios,
                                NULL /*AllocationSize*/,
                                fDirAttribs,
                                FILE_SHARE_READ | FILE_SHARE_WRITE,
                                FILE_CREATE,
                                FILE_DIRECTORY_FILE | FILE_OPEN_FOR_BACKUP_INTENT | FILE_SYNCHRONOUS_IO_NONALERT,
                                NULL /*EaBuffer*/,
                                0 /*EaLength*/);
        }

        if (NT_SUCCESS(rcNt))
        {
            if (!phSubDir)
            {
                NtClose(hNewDir);
                rc = VINF_SUCCESS;
            }
            else
            {
                rc = rtDirOpenRelativeOrHandle(phSubDir, pszRelPath, RTDIRFILTER_NONE, 0 /*fFlags*/,
                                               (uintptr_t)hNewDir, NULL /*pvNativeRelative*/);
                if (RT_FAILURE(rc))
                    NtClose(hNewDir);
            }
        }
        else
            rc = RTErrConvertFromNtStatus(rcNt);
        RTNtPathFree(&NtName, NULL);
    }
RTDECL(int) RTDirRelDirRemove(RTDIR hDir, const char *pszRelPath)
    PRTDIRINTERNAL pThis = hDir;
    /*
     * Convert and normalize the path.
     */
    UNICODE_STRING NtName;
    HANDLE hRoot = pThis->hDir;
    int rc = RTNtPathRelativeFromUtf8(&NtName, &hRoot, pszRelPath, RTDIRREL_NT_GET_ASCENT(pThis),
                                      pThis->enmInfoClass == FileMaximumInformation);
    {
        HANDLE              hSubDir = RTNT_INVALID_HANDLE_VALUE;
        IO_STATUS_BLOCK     Ios     = RTNT_IO_STATUS_BLOCK_INITIALIZER;
        OBJECT_ATTRIBUTES   ObjAttr;
        InitializeObjectAttributes(&ObjAttr, &NtName, 0 /*fAttrib*/, hRoot, NULL);

        NTSTATUS rcNt = NtCreateFile(&hSubDir,
                                     DELETE | SYNCHRONIZE,
                                     &ObjAttr,
                                     &Ios,
                                     NULL /*AllocationSize*/,
                                     FILE_ATTRIBUTE_NORMAL,
                                     FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
                                     FILE_OPEN,
                                     FILE_DIRECTORY_FILE | FILE_OPEN_FOR_BACKUP_INTENT | FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_REPARSE_POINT,
                                     NULL /*EaBuffer*/,
                                     0 /*EaLength*/);
        if (NT_SUCCESS(rcNt))
        {
            FILE_DISPOSITION_INFORMATION DispInfo;
            DispInfo.DeleteFile = TRUE;
            RTNT_IO_STATUS_BLOCK_REINIT(&Ios);
            rcNt = NtSetInformationFile(hSubDir, &Ios, &DispInfo, sizeof(DispInfo), FileDispositionInformation);

            NTSTATUS rcNt2 = NtClose(hSubDir);
            if (!NT_SUCCESS(rcNt2) && NT_SUCCESS(rcNt))
                rcNt = rcNt2;
        }

        if (NT_SUCCESS(rcNt))
            rc = VINF_SUCCESS;
        else
            rc = RTErrConvertFromNtStatus(rcNt);

        RTNtPathFree(&NtName, NULL);
    }
RTDECL(int) RTDirRelPathQueryInfo(RTDIR hDir, const char *pszRelPath, PRTFSOBJINFO pObjInfo,
    PRTDIRINTERNAL pThis = hDir;
    /*
     * Validate and convert flags.
     */
    UNICODE_STRING NtName;
    HANDLE hRoot = pThis->hDir;
    int rc = RTNtPathRelativeFromUtf8(&NtName, &hRoot, pszRelPath, RTDIRREL_NT_GET_ASCENT(pThis),
                                      pThis->enmInfoClass == FileMaximumInformation);
    {
        rc = rtPathNtQueryInfoWorker(hRoot, &NtName, pObjInfo, enmAddAttr, fFlags, pszRelPath);
        RTNtPathFree(&NtName, NULL);
    }
RTDECL(int) RTDirRelPathSetMode(RTDIR hDir, const char *pszRelPath, RTFMODE fMode, uint32_t fFlags)
    PRTDIRINTERNAL pThis = hDir;
RTAssertMsg2("DBG: RTDirRelPathSetMode(%s)...\n", szPath);
RTDECL(int) RTDirRelPathSetTimes(RTDIR hDir, const char *pszRelPath, PCRTTIMESPEC pAccessTime, PCRTTIMESPEC pModificationTime,
    PRTDIRINTERNAL pThis = hDir;
    {
RTAssertMsg2("DBG: RTDirRelPathSetTimes(%s)...\n", szPath);
    }
RTDECL(int) RTDirRelPathSetOwner(RTDIR hDir, const char *pszRelPath, uint32_t uid, uint32_t gid, uint32_t fFlags)
    PRTDIRINTERNAL pThis = hDir;
RTAssertMsg2("DBG: RTDirRelPathSetOwner(%s)...\n", szPath);
RTDECL(int) RTDirRelPathRename(RTDIR hDirSrc, const char *pszSrc, RTDIR hDirDst, const char *pszDst, unsigned fRename)
    PRTDIRINTERNAL pThis = hDirSrc;
    PRTDIRINTERNAL pThat = hDirDst;
        {
RTAssertMsg2("DBG: RTDirRelPathRename(%s,%s)...\n", szSrcPath, szDstPath);
            rc = RTPathRename(szSrcPath, szDstPath, fRename);
        }
RTDECL(int) RTDirRelPathUnlink(RTDIR hDir, const char *pszRelPath, uint32_t fUnlink)
    PRTDIRINTERNAL pThis = hDir;
    {
RTAssertMsg2("DBG: RTDirRelPathUnlink(%s)...\n", szPath);
    }
RTDECL(int) RTDirRelSymlinkCreate(RTDIR hDir, const char *pszSymlink, const char *pszTarget,
    PRTDIRINTERNAL pThis = hDir;
    {
RTAssertMsg2("DBG: RTDirRelSymlinkCreate(%s)...\n", szPath);
    }
RTDECL(int) RTDirRelSymlinkRead(RTDIR hDir, const char *pszSymlink, char *pszTarget, size_t cbTarget, uint32_t fRead)
    PRTDIRINTERNAL pThis = hDir;
    {
RTAssertMsg2("DBG: RTDirRelSymlinkRead(%s)...\n", szPath);
    }