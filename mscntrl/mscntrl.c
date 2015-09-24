/* mscntrl.c */

#pragma strings(readonly)

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#define INCL_BASE
#define INCL_DOSDEVIOCTL
#define INCL_ERRORS
#include <os2.h>

/*  */
#define CDROMDMD_NAME "\\DEV\\CD-ROM2$"
#define IOCTL_CDROMDISK2 0x82
#define CD01 0x31304443L

/*  */
#define CDROMDISK_FEATURE_CTRL 0x4a
struct FeatureCtrl_Parm
{
    ULONG ID_code;
    USHORT flags;
};
#define FEATURECTRL_DISABLEMS  0x0001
#define FEATURECTRL_FIXTRK1NEG 0x0002

/*  */
struct DeviceStatus_Parm
{
    ULONG ID_code;
};
struct DeviceStatus_Data
{
    ULONG device_status;
};
#define DSF_FIX_TRACK1NEGATIVE 0x08000000L
#define DSF_MS_DISABLED        0x10000000L
#define DSF_MS_SUPPORT         0x20000000L

/*  */
#define CDROMDISK2_DRIVELETTERS 0x60
struct DriveLetters_Data
{
    USHORT drive_count;
    USHORT first_drive;
};

/*  */
#define CDROMDISK2_FEATURES 0x63
struct Features_Data
{
    ULONG driver_status;
};
#define FEATURE_MSCTRL_SUPPORT     0x00008000L
#define FEATURE_TRK1NEGFIX_SUPPORT 0x00004000L

/*  */
struct CDROMDMDInfo_t
{
    UCHAR ucDriveFirst;
    ULONG ulDriveCount;
    ULONG ulFeatureFlags;
};
static APIRET QueryCDROMDMDInfo(struct CDROMDMDInfo_t* pxInfo)
{
    APIRET rc;
    HFILE hfDMD;
    ULONG ulDataLen;
    union
    {
        struct DriveLetters_Data xDRIVELETTERS;
        struct Features_Data xFEATURES;
    } xData;
    rc = DosOpen(CDROMDMD_NAME,
                 &hfDMD,
                 &ulDataLen,
                 (ULONG)0,
                 FILE_NORMAL,
                 OPEN_ACTION_FAIL_IF_NEW | OPEN_ACTION_OPEN_IF_EXISTS,
                 OPEN_FLAGS_FAIL_ON_ERROR | OPEN_FLAGS_NOINHERIT | OPEN_SHARE_DENYNONE | OPEN_ACCESS_READONLY,
                 (PEAOP2)NULL);
    if(rc == NO_ERROR)
    {
        rc = DosDevIOCtl(hfDMD,
                         IOCTL_CDROMDISK2,
                         CDROMDISK2_DRIVELETTERS,
                         (PVOID)NULL,
                         (ULONG)0,
                         (PULONG)NULL,
                         (PVOID)&xData.xDRIVELETTERS,
                         ulDataLen = (ULONG)sizeof(xData.xDRIVELETTERS),
                         &ulDataLen);
        if(rc == NO_ERROR)
        {
            pxInfo->ucDriveFirst = (UCHAR)xData.xDRIVELETTERS.first_drive + (UCHAR)'A';
            pxInfo->ulDriveCount = (ULONG)xData.xDRIVELETTERS.drive_count;
            pxInfo->ulFeatureFlags = (ULONG)0;
            if(DosDevIOCtl(hfDMD,
                           IOCTL_CDROMDISK2,
                           CDROMDISK2_FEATURES,
                           (PVOID)NULL,
                           (ULONG)0,
                           (PULONG)NULL,
                           (PVOID)&xData.xFEATURES,
                           ulDataLen = (ULONG)sizeof(xData.xFEATURES),
                           &ulDataLen) == NO_ERROR)
                pxInfo->ulFeatureFlags = xData.xFEATURES.driver_status;
        }
        (VOID)DosClose(hfDMD);
    }
    return rc;
}

/*  */
static APIRET OpenDrive(PHFILE phfDrive,
                        UCHAR ucDrive)
{
    UCHAR aucPath[4];
    ULONG ulWork;
    *(PULONG)&aucPath[0] = (ULONG)ucDrive + ((ULONG)(UCHAR)':' << 8) + ((ULONG)(UCHAR)'\0' << 16) + ((ULONG)(UCHAR)'\0' << 24);
    return DosOpen((PSZ)&aucPath[0],
                   phfDrive,
                   &ulWork,
                   (ULONG)0,
                   FILE_NORMAL,
                   OPEN_ACTION_FAIL_IF_NEW | OPEN_ACTION_OPEN_IF_EXISTS,
                   OPEN_FLAGS_DASD | OPEN_FLAGS_FAIL_ON_ERROR | OPEN_FLAGS_NOINHERIT | OPEN_SHARE_DENYNONE | OPEN_ACCESS_READONLY,
                   (PEAOP2)NULL);
}

/*  */
static APIRET QueryDeviceStatus(HFILE hfDrive,
                                PULONG pulStatus)
{
    APIRET rc;
    ULONG ulParmLen;
    struct DeviceStatus_Parm xParm;
    ULONG ulDataLen;
    struct DeviceStatus_Data xData;
    xParm.ID_code = CD01;
    rc = DosDevIOCtl(hfDrive,
                     IOCTL_CDROMDISK,
                     CDROMDISK_DEVICESTATUS,
                     (PVOID)&xParm,
                     ulParmLen = (ULONG)sizeof(xParm),
                     &ulParmLen,
                     (PVOID)&xData,
                     ulDataLen = (ULONG)sizeof(xData),
                     &ulDataLen);
    if(rc == NO_ERROR)
        *pulStatus = xData.device_status;
    return rc;
}

/*  */
static APIRET SetFeatureControl(HFILE hfDrive,
                                ULONG ulFlags)
{
    ULONG ulParmLen;
    struct FeatureCtrl_Parm xParm;
    xParm.ID_code = CD01;
    xParm.flags = (USHORT)ulFlags;
    return DosDevIOCtl(hfDrive,
                       IOCTL_CDROMDISK,
                       CDROMDISK_FEATURE_CTRL,
                       (PVOID)&xParm,
                       ulParmLen = (ULONG)sizeof(xParm),
                       &ulParmLen,
                       (PVOID)NULL,
                       (ULONG)0,
                       (PULONG)NULL);
}

/*  */
int main(int argc,
         char* argv[])
{
    APIRET rc;
    struct CDROMDMDInfo_t xCDROMDMDInfo;
    UCHAR ucDrive;
    HFILE hfDrive;
    ULONG ulDriveStatus;
    ULONG ulFeatureFlags;

    (void)printf("\n");

    rc = QueryCDROMDMDInfo(&xCDROMDMDInfo);
    if(argc < 2)
    {
        (void)printf("mscntrl.exe:  sets / resets feature flags of specified CD-ROM drive.\n"
                     "usage :\n"
                     "  mscntrl  {CD-ROM drive letter}  [command]\n"
                     "commands : (CaSe SeNsItIvE)\n"
                     "  ms_disable / ms_enable   :\n"
                     "    disable / enable multisession handling\n"
                     "  nt1_disable / nt1_enable :\n"
                     "    disable / enable 'track 1 negative start LBA fix'\n");
        if(rc != NO_ERROR)
            (void)printf("CD-ROM device manager driver is not installed.\n");
        else
        {
            (void)printf("drive letter mapping:\n"
                         "  %c: - %c:\n"
                         "supported features :\n"
                         "  multisessioning disable control : %s\n"
                         "  track 1 negative start LBA fix  : %s\n",
                         (int)xCDROMDMDInfo.ucDriveFirst,
                         (int)(xCDROMDMDInfo.ucDriveFirst + (UCHAR)xCDROMDMDInfo.ulDriveCount - (UCHAR)1),
                         (xCDROMDMDInfo.ulFeatureFlags & FEATURE_MSCTRL_SUPPORT) != (ULONG)0 ? "yes" : "no",
                         (xCDROMDMDInfo.ulFeatureFlags & FEATURE_TRK1NEGFIX_SUPPORT) != (ULONG)0 ? "yes" : "no");
        }
    }
    else
    {
        ucDrive = (UCHAR)argv[1][0] & (UCHAR)~('a' - 'A');
        if(ucDrive < xCDROMDMDInfo.ucDriveFirst ||
           ucDrive > xCDROMDMDInfo.ucDriveFirst + (UCHAR)xCDROMDMDInfo.ulDriveCount - (UCHAR)1)
            (void)printf("mscntrl : CD-ROM drive letter is out of range.\n");
        else
        {
            rc = OpenDrive(&hfDrive,
                           ucDrive);
            if(rc != NO_ERROR)
                (void)printf("mscntrl : cannot open drive %c: (rc=%u)\n",
                             (int)ucDrive,
                             (unsigned int)rc);
            else
            {
                rc = QueryDeviceStatus(hfDrive,
                                       &ulDriveStatus);
                if(rc != NO_ERROR)
                    (void)printf("mscntrl : cannot determine current status of drive %c: (rc=%u)\n",
                                 (int)ucDrive,
                                 (unsigned int)rc);
                else
                {
                    (void)printf("current status of drive %c:\n"
                                 "  multisession handling          : %s\n"
                                 "  track 1 negative start LBA fix : %s\n",
                                 (int)ucDrive,
                                 (xCDROMDMDInfo.ulFeatureFlags & FEATURE_MSCTRL_SUPPORT) != (ULONG)0 ? ((ulDriveStatus & DSF_MS_DISABLED) != (ULONG)0 ? "disabled" : "enabled")
                                                                                                     : "not supported",
                                 (xCDROMDMDInfo.ulFeatureFlags & FEATURE_TRK1NEGFIX_SUPPORT) != (ULONG)0 ? ((ulDriveStatus & DSF_FIX_TRACK1NEGATIVE) != (ULONG)0 ? "enabled" : "disabled")
                                                                                                         : "not supported");
                    if(argc > 2)
                    {
                        ulFeatureFlags = (ULONG)0;
                        if((ulDriveStatus & DSF_MS_DISABLED) != (ULONG)0)
                            ulFeatureFlags |= FEATURECTRL_DISABLEMS;
                        if((ulDriveStatus & DSF_FIX_TRACK1NEGATIVE) != (ULONG)0)
                            ulFeatureFlags |= FEATURECTRL_FIXTRK1NEG;
                        if(strcmp((const char*)argv[2],
                                  "ms_disable") == 0)
                            ulFeatureFlags |= FEATURECTRL_DISABLEMS;
                        else if(strcmp((const char*)argv[2],
                                       "ms_enable") == 0)
                            ulFeatureFlags &= ~FEATURECTRL_DISABLEMS;
                        else if(strcmp((const char*)argv[2],
                                       "nt1_disable") == 0)
                            ulFeatureFlags &= ~FEATURECTRL_FIXTRK1NEG;
                        else if(strcmp((const char*)argv[2],
                                       "nt1_enable") == 0)
                            ulFeatureFlags |= FEATURECTRL_FIXTRK1NEG;
                        rc = SetFeatureControl(hfDrive,
                                               ulFeatureFlags);
                        if(rc != NO_ERROR)
                            (void)printf("mscntrl : cannot set feature control flags of drive %c: (rc=%u)\n",
                                         (int)ucDrive,
                                         (unsigned int)rc);
                        else
                            (void)printf("new status of drive %c:\n"
                                         "  multisession handling          : %s\n"
                                         "  track 1 negative start LBA fix : %s\n",
                                         (int)ucDrive,
                                         (xCDROMDMDInfo.ulFeatureFlags & FEATURE_MSCTRL_SUPPORT) != (ULONG)0 ? ((ulFeatureFlags & FEATURECTRL_DISABLEMS) != (ULONG)0 ? "disabled" : "enabled")
                                                                                                             : "not supported",
                                         (xCDROMDMDInfo.ulFeatureFlags & FEATURE_TRK1NEGFIX_SUPPORT) != (ULONG)0 ? ((ulFeatureFlags & FEATURECTRL_FIXTRK1NEG) != (ULONG)0 ? "enabled" : "disabled")
                                                                                                                 : "not supported");
                    }
                }
                (VOID)DosClose(hfDrive);
            }
        }
    }

    return 0;
}

