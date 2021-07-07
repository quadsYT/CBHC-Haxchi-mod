/*
 * Copyright (C) 2016-2017 FIX94
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */
#include <string.h>
#include "types.h"
#include "coreinit.h"
#include "pad.h"
#include "../global.h"

static unsigned int getButtonsDown(unsigned int padscore_handle, unsigned int vpad_handle);

#define BUS_SPEED                       248625000
#define SECS_TO_TICKS(sec)              (((unsigned long long)(sec)) * (BUS_SPEED/4))
#define MILLISECS_TO_TICKS(msec)        (SECS_TO_TICKS(msec) / 1000)
#define MICROSECS_TO_TICKS(usec)        (SECS_TO_TICKS(usec) / 1000000)

#define usleep(usecs)                   OSSleepTicks(MICROSECS_TO_TICKS(usecs))
#define sleep(secs)                     OSSleepTicks(SECS_TO_TICKS(secs))

#define FORCE_SYSMENU (VPAD_BUTTON_UP)
#define FORCE_HBL (VPAD_BUTTON_A)
#define FORCE_MOCHA (VPAD_BUTTON_Y)
#define FORCE_FTP (VPAD_BUTTON_L)
#define FORCE_APPSTORE (VPAD_BUTTON_R)
#define FORCE_WUP (VPAD_BUTTON_ZL)
#define FORCE_HIDTOVPAD (VPAD_BUTTON_ZR)
#define FORCE_SWAPDRC (VPAD_BUTTON_PLUS)
#define FORCE_TCPGECKO (VPAD_BUTTON_MINUS)
#define FORCE_CUSTOM (VPAD_BUTTON_DOWN)
#define SD_HBL_PATH "/vol/external01/wiiu/apps/homebrew_launcher/homebrew_launcher.elf"
#define SD_MOCHA_PATH "/vol/external01/wiiu/apps/mocha/mocha.elf"
#define SD_FTP_PATH "/vol/external01/wiiu/apps/fpiiu-cbhc/ftpiiu.elf"
#define SD_SDCAFIINE_PATH "/vol/external01/wiiu/apps/SDcafiine/sdcafiine.elf"
#define SD_HIDTOVPAD_PATH "/storage_mlc/sys/homebrewnand/hidtovpad.elf"
#define SD_APPSTORE_PATH "/vol/external01/wiiu/apps/appstore/hbas.elf"
#define SD_WUP_PATH "/vol/external01/wiiu/apps/wup_installer_gx2/wup_installer_gx2.elf"
#define SD_SWAPDRC_PATH "/vol/external01/wiiu/apps/swapdrc/swapdrc.elf"
#define SD_TCPGECKO_PATH "/vol/external01/wiiu/apps/tcpgecko/tcpgecko.elf"
#define SD_CUSTOM_PATH "/vol/external01/wiiu/apps/custom/custom.elf"

static const char *verChar = "v1.6             .:ColdBoot HaxChi Menu:.          ";
static const char *verChar2 = "                  .:Quickboot Buttons:.            ";
static const unsigned long long VWII_SYSMENU_TID = 0x0000000100000002ULL;
static const unsigned long long VWII_HBC_TID = 0x000100014C554C5AULL;

#define DEFAULT_DISABLED 0
#define DEFAULT_SYSMENU 1
#define DEFAULT_MOCHA 2
#define DEFAULT_CFW_IMG 3
#define DEFAULT_HBL 4
#define DEFAULT_APPSTORE 5
#define DEFAULT_SDCAFIINE 6
#define DEFAULT_HIDTOVPAD 7
#define DEFAULT_FTP 8
#define DEFAULT_WUP 9
#define DEFAULT_SWAPDRC 10
#define DEFAULT_TCPGECKO 11
#define DEFAULT_CUSTOM 12
#define DEFAULT_VWII_SYSMENU 13
#define DEFAULT_VWII_HBC 14
#define DEFAULT_MAX 15

static const char *defOpts[DEFAULT_MAX] = {
	"DEFAULT_DISABLED",
	"DEFAULT_SYSMENU",
	"DEFAULT_MOCHA",
	"DEFAULT_CFW_IMG",
	"DEFAULT_HBL",
	"DEFAULT_APPSTORE"
	"DEFAULT_SDCAFIINE",
	"DEFAULT_HIDTOVPAD",
	"DEFAULT_FTP",
	"DEFAULT_WUP",
	"DEFAULT_SWAPDRC",
	"DEFAULT_TCPGECKO",
	"DEFAULT_CUSTOM",
	"DEFAULT_VWII_SYSMENU",
	"DEFAULT_VWII_HBC",
};

static const char *bootOpts[DEFAULT_MAX] = {
	"Disabled",
	"WiiU CBHC CFW (recommended)",
	"WiiU Mocha CFW",
	"WiiU SD CFW",
	"WiiU Homebrew Launcher",
	"WiiU Homebrew App Store",
	"WiiU SDcafiine",
	"WiiU HID TO VPAD",
	"WiiU FTPiiU Everywhere",
	"WiiU WUP Installer GX2",
	"WiiU SwapDRC",
	"WiiU TCP Gecko",
	"WiiU Custom .elf",
	"vWii System Menu",
	"vWii Homebrew Channel",
};

#define OSScreenEnable(enable) OSScreenEnableEx(0, enable); OSScreenEnableEx(1, enable);
#define OSScreenClearBuffer(tmp) OSScreenClearBufferEx(0, tmp); OSScreenClearBufferEx(1, tmp);
#define OSScreenPutFont(x, y, buf) OSScreenPutFontEx(0, x, y, buf); OSScreenPutFontEx(1, x, y, buf);
#define OSScreenFlipBuffers() OSScreenFlipBuffersEx(0); OSScreenFlipBuffersEx(1);

uint32_t __main(void)
{
	/* coreinit functions */
	unsigned int coreinit_handle;
	OSDynLoad_Acquire("coreinit.rpl", &coreinit_handle);

	/* coreinit os functions*/
	int (*OSForceFullRelaunch)(void);
	void (*OSSleepTicks)(unsigned long long ticks);
	void (*OSExitThread)(int);
	unsigned long long(*OSGetTitleID)();

	OSDynLoad_FindExport(coreinit_handle, 0, "OSForceFullRelaunch", &OSForceFullRelaunch);
	OSDynLoad_FindExport(coreinit_handle, 0, "OSSleepTicks", &OSSleepTicks);
	OSDynLoad_FindExport(coreinit_handle, 0, "OSExitThread", &OSExitThread);
	OSDynLoad_FindExport(coreinit_handle, 0, "OSGetTitleID", &OSGetTitleID);

	/* coreinit os screen functions */
	void(*OSScreenInit)();
	void(*OSScreenEnableEx)(unsigned int bufferNum, int enable);
	unsigned int(*OSScreenGetBufferSizeEx)(unsigned int bufferNum);
	unsigned int(*OSScreenSetBufferEx)(unsigned int bufferNum, void * addr);
	unsigned int(*OSScreenClearBufferEx)(unsigned int bufferNum, unsigned int temp);
	unsigned int(*OSScreenPutFontEx)(unsigned int bufferNum, unsigned int posX, unsigned int posY, const char * buffer);
	unsigned int(*OSScreenFlipBuffersEx)(unsigned int bufferNum);

	OSDynLoad_FindExport(coreinit_handle, 0, "OSScreenInit", &OSScreenInit);
	OSDynLoad_FindExport(coreinit_handle, 0, "OSScreenEnableEx", &OSScreenEnableEx);
	OSDynLoad_FindExport(coreinit_handle, 0, "OSScreenGetBufferSizeEx", &OSScreenGetBufferSizeEx);
	OSDynLoad_FindExport(coreinit_handle, 0, "OSScreenSetBufferEx", &OSScreenSetBufferEx);
	OSDynLoad_FindExport(coreinit_handle, 0, "OSScreenClearBufferEx", &OSScreenClearBufferEx);
	OSDynLoad_FindExport(coreinit_handle, 0, "OSScreenPutFontEx", &OSScreenPutFontEx);
	OSDynLoad_FindExport(coreinit_handle, 0, "OSScreenFlipBuffersEx", &OSScreenFlipBuffersEx);

	/* coreinit memory functions */
	void (*DCStoreRange)(const void *addr, uint32_t length);
	unsigned int *pMEMAllocFromDefaultHeapEx;
	unsigned int *pMEMFreeToDefaultHeap;
	OSDynLoad_FindExport(coreinit_handle, 0, "DCStoreRange", &DCStoreRange);
	OSDynLoad_FindExport(coreinit_handle, 1, "MEMAllocFromDefaultHeapEx", &pMEMAllocFromDefaultHeapEx);
	OSDynLoad_FindExport(coreinit_handle, 1, "MEMFreeToDefaultHeap", &pMEMFreeToDefaultHeap);

	void* (*MEMAllocFromDefaultHeapEx)(int size, int align) = (void*)(*pMEMAllocFromDefaultHeapEx);
	void (*MEMFreeToDefaultHeap)(void *ptr) = (void*)(*pMEMFreeToDefaultHeap);

	/* coreinit fs functions */
	int(*FSInit)(void);
	void(*FSShutdown)(void);
	int(*FSAddClient)(void *pClient, int errHandling);
	int(*FSDelClient)(void *pClient);
	void(*FSInitCmdBlock)(void *pCmd);
	int(*FSWriteFile)(void *pClient, void *pCmd, const void *buffer, int size, int count, int fd, int flag, int errHandling);
	int(*FSCloseFile)(void *pClient, void *pCmd, int fd, int errHandling);

	OSDynLoad_FindExport(coreinit_handle, 0, "FSInit", &FSInit);
	OSDynLoad_FindExport(coreinit_handle, 0, "FSShutdown", &FSShutdown);
	OSDynLoad_FindExport(coreinit_handle, 0, "FSInitCmdBlock", &FSInitCmdBlock);
	OSDynLoad_FindExport(coreinit_handle, 0, "FSAddClient", &FSAddClient);
	OSDynLoad_FindExport(coreinit_handle, 0, "FSDelClient", &FSDelClient);
	OSDynLoad_FindExport(coreinit_handle, 0, "FSWriteFile", &FSWriteFile);
	OSDynLoad_FindExport(coreinit_handle, 0, "FSCloseFile", &FSCloseFile);

	/* act functions */
	unsigned int act_handle;
	OSDynLoad_Acquire("nn_act.rpl", &act_handle);

	void(*nn_act_initialize)(void);
	unsigned char(*nn_act_getslotno)(void);
	unsigned char(*nn_act_getdefaultaccount)(void);
	void(*nn_act_finalize)(void);

	OSDynLoad_FindExport(act_handle, 0, "Initialize__Q2_2nn3actFv", &nn_act_initialize);
	OSDynLoad_FindExport(act_handle, 0, "GetSlotNo__Q2_2nn3actFv", &nn_act_getslotno);
	OSDynLoad_FindExport(act_handle, 0, "GetDefaultAccount__Q2_2nn3actFv", &nn_act_getdefaultaccount);
	OSDynLoad_FindExport(act_handle, 0, "Finalize__Q2_2nn3actFv", &nn_act_finalize);

	/* padscore functions */
	unsigned int padscore_handle;
	OSDynLoad_Acquire("padscore.rpl", &padscore_handle);

	void(*WPADEnableURCC)(int enable);
	void(*KPADSetConnectCallback)(int chan, void *ptr);
	void*(*WPADSetSyncDeviceCallback)(void *ptr);
	void(*KPADShutdown)(void);
	//easly allows us callback without execute permission on other cores
	char(*WPADGetSpeakerVolume)(void);
	void(*WPADSetSpeakerVolume)(char);

	OSDynLoad_FindExport(padscore_handle, 0, "WPADEnableURCC", &WPADEnableURCC);
	OSDynLoad_FindExport(padscore_handle, 0, "KPADSetConnectCallback", &KPADSetConnectCallback);
	OSDynLoad_FindExport(padscore_handle, 0, "WPADSetSyncDeviceCallback", &WPADSetSyncDeviceCallback);
	OSDynLoad_FindExport(padscore_handle, 0, "KPADShutdown",&KPADShutdown);
	OSDynLoad_FindExport(padscore_handle, 0, "WPADGetSpeakerVolume", &WPADGetSpeakerVolume);
	OSDynLoad_FindExport(padscore_handle, 0, "WPADSetSpeakerVolume", &WPADSetSpeakerVolume);

	/* save functions */
	unsigned int save_handle;
	OSDynLoad_Acquire("nn_save.rpl", &save_handle);

	void(*SAVEInit)(void);
	void(*SAVEShutdown)(void);
	void(*SAVEInitSaveDir)(unsigned char user);
	int(*SAVEOpenFile)(void *pClient, void *pCmd, unsigned char user, const char *path, const char *mode, int *fd, int errHandling);
	int(*SAVEFlushQuota)(void *pClient, void *pCmd, unsigned char user, int errHandling);
	void(*SAVERename)(void *pClient, void *pCmd, unsigned char user, const char *oldpath, const char *newpath, int errHandling);

	OSDynLoad_FindExport(save_handle, 0, "SAVEInit",&SAVEInit);
	OSDynLoad_FindExport(save_handle, 0, "SAVEShutdown",&SAVEShutdown);
	OSDynLoad_FindExport(save_handle, 0, "SAVEInitSaveDir",&SAVEInitSaveDir);
	OSDynLoad_FindExport(save_handle, 0, "SAVEOpenFile", &SAVEOpenFile);
	OSDynLoad_FindExport(save_handle, 0, "SAVEFlushQuota", &SAVEFlushQuota);
	OSDynLoad_FindExport(save_handle, 0, "SAVERename", &SAVERename);

	/* sysapp functions */
	unsigned int sysapp_handle;
	OSDynLoad_Acquire("sysapp.rpl", &sysapp_handle);

	void (*SYSLaunchMenu)(void);
	void(*_SYSLaunchMenuWithCheckingAccount)(unsigned char slot);
	int(*_SYSLaunchTitleWithStdArgsInNoSplash)(unsigned long long tid, void *ptr);
	unsigned long long(*_SYSGetSystemApplicationTitleId)(int sysApp);

	OSDynLoad_FindExport(sysapp_handle, 0, "SYSLaunchMenu", &SYSLaunchMenu);
	OSDynLoad_FindExport(sysapp_handle, 0, "_SYSLaunchMenuWithCheckingAccount", &_SYSLaunchMenuWithCheckingAccount);
	OSDynLoad_FindExport(sysapp_handle, 0, "_SYSLaunchTitleWithStdArgsInNoSplash", &_SYSLaunchTitleWithStdArgsInNoSplash);
	OSDynLoad_FindExport(sysapp_handle, 0, "_SYSGetSystemApplicationTitleId", &_SYSGetSystemApplicationTitleId);

	/* vpad functions */
	unsigned int vpad_handle;
	OSDynLoad_Acquire("vpad.rpl", &vpad_handle);

	int(*VPADRead)(int controller, VPADData *buffer, unsigned int num, int *error);

	OSDynLoad_FindExport(vpad_handle, 0, "VPADRead", &VPADRead);

	/* set up some variables */
	int launchmode = LAUNCH_SYSMENU;
	unsigned int dsvcid = (unsigned int)(OSGetTitleID(0) & 0xFFFFFFFF);
	unsigned long long sysmenu = _SYSGetSystemApplicationTitleId(0);

	nn_act_initialize();
	unsigned char slot = nn_act_getslotno();
	unsigned char defaultSlot = nn_act_getdefaultaccount();
	nn_act_finalize();

	/* pre-menu button combinations which can be held on gamepad */
	int vpadError = -1;
	VPADData vpad;
	VPADRead(0, &vpad, 1, &vpadError);
	if(vpadError == 0)
	{
		if(((vpad.btns_d|vpad.btns_h) & FORCE_SYSMENU) == FORCE_SYSMENU)
		{
			// iosuhax-less menu launch backup code
			_SYSLaunchTitleWithStdArgsInNoSplash(sysmenu, 0);
			OSExitThread(0);
			return 0;
		}
		else if(((vpad.btns_d|vpad.btns_h) & FORCE_HBL) == FORCE_HBL)
		{
			// original hbl loader payload
			strcpy((void*)0xF5E70000,SD_HBL_PATH);
			return 0x01800000;
		}
		else if(((vpad.btns_d|vpad.btns_h) & FORCE_FTP) == FORCE_FTP)
		{
			// ftp loader payload
			strcpy((void*)0xF5E70000,SD_FTP_PATH);
			return 0x01800000;
		}
		else if(((vpad.btns_d|vpad.btns_h) & FORCE_MOCHA) == FORCE_MOCHA)
		{
			// mocha loader payload
			strcpy((void*)0xF5E70000,SD_MOCHA_PATH);
			return 0x01800000;
		}
		else if(((vpad.btns_d|vpad.btns_h) & FORCE_WUP) == FORCE_WUP)
		{
			// wup loader payload
			strcpy((void*)0xF5E70000,SD_WUP_PATH);
			return 0x01800000;
		}
		else if(((vpad.btns_d|vpad.btns_h) & FORCE_SWAPDRC) == FORCE_SWAPDRC)
		{
			// swapdrc loader payload
			strcpy((void*)0xF5E70000,SD_SWAPDRC_PATH);
			return 0x01800000;
		}
		else if(((vpad.btns_d|vpad.btns_h) & FORCE_TCPGECKO) == FORCE_TCPGECKO)
		{
			// tcpgecko loader payload
			strcpy((void*)0xF5E70000,SD_TCPGECKO_PATH);
			return 0x01800000;
		}
		else if(((vpad.btns_d|vpad.btns_h) & FORCE_APPSTORE) == FORCE_APPSTORE)
		{
			// appstore loader payload
			strcpy((void*)0xF5E70000,SD_APPSTORE_PATH);
			return 0x01800000;
		}
		else if(((vpad.btns_d|vpad.btns_h) & FORCE_CUSTOM) == FORCE_CUSTOM)
		{
			// custom loader payload
			strcpy((void*)0xF5E70000,SD_CUSTOM_PATH);
			return 0x01800000;
		}
		else if(((vpad.btns_d|vpad.btns_h) & FORCE_HIDTOVPAD) == FORCE_HIDTOVPAD)
		{
			// hidtovpad loader payload
			strcpy((void*)0xF5E70000,SD_HIDTOVPAD_PATH);
			return 0x01800000;
		}
		else if((vpad.btns_d|vpad.btns_h) == VPAD_BUTTON_B)
		{
			launchmode = LAUNCH_VWII_SYSMENU;
			goto do_launch_selection;
		}
		else if((vpad.btns_d|vpad.btns_h) == VPAD_BUTTON_X)
		{
			launchmode = LAUNCH_VWII_HBC;
			goto do_launch_selection;
		}		
	}

//cbhc_menu_start:
	void *pClient = MEMAllocFromDefaultHeapEx(0x1700,4);
	void *pCmd = MEMAllocFromDefaultHeapEx(0xA80,4);

	//prepare FS and SAVE API
	FSInit();
	SAVEInit();
	SAVEInitSaveDir(slot);
	FSAddClient(pClient, -1);
	FSInitCmdBlock(pCmd);

	//check for autoboot file; if not found create one 
	int autoboot = -1;
	int iFd = -1;
	int i;
	for(i = 0; i < DEFAULT_MAX; i++)
	{
		SAVEOpenFile(pClient, pCmd, slot, defOpts[i], "r", &iFd, -1);
		if (iFd >= 0)
		{
			autoboot = i;
			FSCloseFile(pClient, pCmd, iFd, -1);
			break;
		}
	}
	if(autoboot < 0)
	{
		autoboot = DEFAULT_DISABLED;
		SAVEOpenFile(pClient, pCmd, slot, defOpts[DEFAULT_DISABLED], "w", &iFd, -1);
		if (iFd >= 0)
			FSCloseFile(pClient, pCmd, iFd, -1);
	}
	if(autoboot > 0)
		launchmode = (autoboot - 1);
	int cur_autoboot = autoboot;

	//fire up screens
	OSScreenInit();
	int screen_buf0_size = OSScreenGetBufferSizeEx(0);
	OSScreenSetBufferEx(0, (void*)(0xF4000000));
	OSScreenSetBufferEx(1, (void*)(0xF4000000 + screen_buf0_size));
	OSScreenEnable(1);

	char verInfStr[64];
	__os_snprintf(verInfStr,64,"%s (%08X)", verChar, dsvcid);
	
	char verInfStr2[64];
	__os_snprintf(verInfStr2,64,"%s", verChar2);

	//enable wiiu pro controller connection
	WPADEnableURCC(1);
	//hachihachi instantly disconnects wiimotes normally
	KPADSetConnectCallback(0,NULL);
	KPADSetConnectCallback(1,NULL);
	KPADSetConnectCallback(2,NULL);
	KPADSetConnectCallback(3,NULL);
	char oriVol = WPADGetSpeakerVolume();
	//WPAD_SYNC_EVT=0 is button pressed
	WPADSetSpeakerVolume(1);
	WPADSetSyncDeviceCallback(WPADSetSpeakerVolume);

	//no autoboot, straight to menu
	if(autoboot == DEFAULT_DISABLED)
		goto cbhc_menu;

	//autoboot wait message
	OSScreenClearBuffer(0);
	OSScreenPutFont(0, 0, verInfStr2);
	OSScreenPutFont(0, 1, " ");
	OSScreenPutFont(0, 2, "Autobooting...");
	OSScreenPutFont(0, 3, " ");
	OSScreenPutFont(0, 4, " Home : CBHC Menu (Including Autoboot Settings)");	
	OSScreenPutFont(0, 5, "    A : Homebrew Launcher");
	OSScreenPutFont(0, 6, "    B : vWII System Menu");
	OSScreenPutFont(0, 7, "    X : vWII Homebrew");
	OSScreenPutFont(0, 8, "    Y : Mocha CFW");
	OSScreenPutFont(0, 9, "    L : FTPiiU Everywhere");
	OSScreenPutFont(0, 10, "    R : Homebrew App Store");
	OSScreenPutFont(0, 11, "    + : SwapDRC");
	OSScreenPutFont(0, 12, "    - : TCP Gecko");
	OSScreenPutFont(0, 13, "   ZL : WUP Intaller GX2");
	OSScreenPutFont(0, 14, "   ZR : HID to VPAD");
	OSScreenPutFont(0, 15, " Down : Custom .elf file (/custom/custom.elf)");
	OSScreenFlipBuffers();

	//garbage read
	getButtonsDown(padscore_handle, vpad_handle);
	//see if menu is requested
	int loadMenu = 0;
	int waitCnt = 40;
	while(waitCnt--)
	{
		unsigned int btnDown = getButtonsDown(padscore_handle, vpad_handle);

		if((btnDown & VPAD_BUTTON_HOME) || WPADGetSpeakerVolume() == 0)
		{
			WPADSetSpeakerVolume(1);
			loadMenu = 1;
			break;
		}
		else if((btnDown & FORCE_SYSMENU) == FORCE_SYSMENU)
		{
			// iosuhax-less menu launch backup code
			_SYSLaunchTitleWithStdArgsInNoSplash(sysmenu, 0);
			OSExitThread(0);
			return 0;
			break;
		}
		else if((btnDown & VPAD_BUTTON_A) == FORCE_HBL)
		{
			// original hbl loader payload
			strcpy((void*)0xF5E70000,SD_HBL_PATH);
			return 0x01800000;
			break;
		}
		else if((btnDown & FORCE_MOCHA) == FORCE_MOCHA)
		{
			// original mocha loader payload
			strcpy((void*)0xF5E70000,SD_MOCHA_PATH);
			return 0x01800000;
			break;
		}
		else if((btnDown & FORCE_FTP) == FORCE_FTP)
		{
			// original ftp loader payload
			strcpy((void*)0xF5E70000,SD_FTP_PATH);
			return 0x01800000;
			break;
		}
		else if((btnDown & FORCE_WUP) == FORCE_WUP)
		{
			// original wup loader payload
			strcpy((void*)0xF5E70000,SD_WUP_PATH);
			return 0x01800000;
			break;
		}
		else if((btnDown & FORCE_SWAPDRC) == FORCE_SWAPDRC)
		{
			// original swapdrc loader payload
			strcpy((void*)0xF5E70000,SD_SWAPDRC_PATH);
			return 0x01800000;
			break;
		}
		else if((btnDown & FORCE_TCPGECKO) == FORCE_TCPGECKO)
		{
			// original tcpgecko loader payload
			strcpy((void*)0xF5E70000,SD_TCPGECKO_PATH);
			return 0x01800000;
			break;
		}
		else if((btnDown & FORCE_CUSTOM) == FORCE_CUSTOM)
		{
			// original custom loader payload
			strcpy((void*)0xF5E70000,SD_CUSTOM_PATH);
			return 0x01800000;
			break;
		}
		else if((btnDown & FORCE_APPSTORE) == FORCE_APPSTORE)
		{
			// original appstore loader payload
			strcpy((void*)0xF5E70000,SD_APPSTORE_PATH);
			return 0x01800000;
			break;
		}
		else if((btnDown & FORCE_HIDTOVPAD) == FORCE_HIDTOVPAD)
		{
			// original hidtovpad loader payload
			strcpy((void*)0xF5E70000,SD_HIDTOVPAD_PATH);
			return 0x01800000;
			break;
		}
		else if((btnDown & VPAD_BUTTON_B) == VPAD_BUTTON_B)
		{
			launchmode = LAUNCH_VWII_SYSMENU;
			goto do_launch_selection;
			break;
			}
		else if((btnDown & VPAD_BUTTON_X) == VPAD_BUTTON_X)
		{
			launchmode = LAUNCH_VWII_HBC;
			goto do_launch_selection;
			break;
			}
		usleep(100000);
	}
	//no menu requested, autoboot
	if(loadMenu == 0)
		goto cbhc_menu_end;

	OSScreenClearBuffer(0);
	OSScreenPutFont(0, 0, verInfStr2);
	OSScreenPutFont(0, 1, " ");
	OSScreenPutFont(0, 2, "Entering Menu...");
	OSScreenFlipBuffers();
	waitCnt = 30;
	while(waitCnt--)
	{
		getButtonsDown(padscore_handle, vpad_handle);
		usleep(50000);
	}

cbhc_menu:	;
	int redraw = 1;
	int PosX = 0;
	int ListMax = 15;
	int clickT = 0;
	while(1)
	{
		unsigned int btnDown = getButtonsDown(padscore_handle, vpad_handle);

		if(WPADGetSpeakerVolume() == 0)
		{
			if(clickT == 0)
				clickT = 8;
			else
			{
				btnDown |= VPAD_BUTTON_A;
				clickT = 0;
			}
			WPADSetSpeakerVolume(1);
		}
		else if(clickT)
		{
			clickT--;
			if(clickT == 0)
				btnDown |= VPAD_BUTTON_DOWN;
		}

		if( btnDown & VPAD_BUTTON_DOWN )
		{
			if(PosX+1 == ListMax)
				PosX = 0;
			else
				PosX++;
			redraw = 1;
		}

		if( btnDown & VPAD_BUTTON_UP )
		{
			if( PosX <= 0 )
				PosX = (ListMax-1);
			else
				PosX--;
			redraw = 1;
		}

		if( btnDown & VPAD_BUTTON_A )
		{
			if(PosX == 14)
			{
				cur_autoboot++;
				if(cur_autoboot == DEFAULT_MAX)
					cur_autoboot = DEFAULT_DISABLED;
				redraw = 1;
			}
			else
			{
				launchmode = PosX;
				break;
			}
		}

		if(redraw)
		{
			OSScreenClearBuffer(0);
			OSScreenPutFont(0, 0, verInfStr);
			char printStr[64];
			OSScreenPutFont(0, 1, " ");
			__os_snprintf(printStr,64,"%c WiiU CBHC CFW", 0 == PosX ? '>' : ' ');
			OSScreenPutFont(0, 2, printStr);
			__os_snprintf(printStr,64,"%c WiiU Mocha CFW", 1 == PosX ? '>' : ' ');
			OSScreenPutFont(0, 3, printStr);
			__os_snprintf(printStr,64,"%c WiiU SD CFW (fw.img)", 2 == PosX ? '>' : ' ');
			OSScreenPutFont(0, 4, printStr);
			__os_snprintf(printStr,64,"%c WiiU Homebrew Launcher", 3 == PosX ? '>' : ' ');
			OSScreenPutFont(0, 5, printStr);
			__os_snprintf(printStr,64,"%c WiiU Homebrew App Store", 4 == PosX ? '>' : ' ');
			OSScreenPutFont(0, 6, printStr);
			__os_snprintf(printStr,64,"%c WiiU SDcafiine", 5 == PosX ? '>' : ' ');
			OSScreenPutFont(0, 7, printStr);
			__os_snprintf(printStr,64,"%c WiiU Hid to VPAD", 6 == PosX ? '>' : ' ');
			OSScreenPutFont(0, 8, printStr);
			__os_snprintf(printStr,64,"%c WiiU FTPiiU Everywhere", 7 == PosX ? '>' : ' ');
			OSScreenPutFont(0, 9, printStr);
			__os_snprintf(printStr,64,"%c WiiU WUP Installer GX2", 8 == PosX ? '>' : ' ');
			OSScreenPutFont(0, 10, printStr);
			__os_snprintf(printStr,64,"%c WiiU SwapDRC", 9 == PosX ? '>' : ' ');
			OSScreenPutFont(0, 11, printStr);
			__os_snprintf(printStr,64,"%c WiiU TCP Gecko", 10 == PosX ? '>' : ' ');
			OSScreenPutFont(0, 12, printStr);
			__os_snprintf(printStr,64,"%c WiiU Custom .elf", 11 == PosX ? '>' : ' ');
			OSScreenPutFont(0, 13, printStr);
			__os_snprintf(printStr,64,"%c vWii System Menu", 12 == PosX ? '>' : ' ');
			OSScreenPutFont(0, 14, printStr);
			__os_snprintf(printStr,64,"%c vWii Homebrew Channel", 13 == PosX ? '>' : ' ');
			OSScreenPutFont(0, 15, printStr)
			OSScreenPutFont(0, 16, " ");
			__os_snprintf(printStr,64,"%c Autoboot: %s", 14 == PosX ? '>' : ' ', bootOpts[cur_autoboot]);
			OSScreenPutFont(0, 17, printStr);

			OSScreenFlipBuffers();
			redraw = 0;
		}
		usleep(50000);
	}
	OSScreenClearBuffer(0);
	OSScreenFlipBuffers();
	usleep(50000);

	//regular menu end, save settings, clean up and launch selection
	cbhc_menu_end:	;
	if(cur_autoboot != autoboot)
		SAVERename(pClient, pCmd, slot, defOpts[autoboot], defOpts[cur_autoboot], -1);

	SAVEFlushQuota(pClient, pCmd, slot, -1);
	FSDelClient(pClient);
	SAVEShutdown();
	FSShutdown();

	MEMFreeToDefaultHeap(pClient);
	MEMFreeToDefaultHeap(pCmd);

	OSScreenClearBuffer(0);
	OSScreenFlipBuffers();

	WPADSetSpeakerVolume(oriVol);

do_launch_selection: ;
	KPADShutdown();

	//store path to sd fw.img for arm_kernel
	if(launchmode == LAUNCH_CFW_IMG)
	{
		strcpy((void*)0xF5E70000,"/vol/sdcard");
		DCStoreRange((void*)0xF5E70000,0x100);
	}

	//do iosu patches
	void (*patch_iosu)(unsigned int coreinit_handle, unsigned int sysapp_handle, int launchmode, int from_cbhc) = (void*)0x01804000;
	patch_iosu(coreinit_handle, sysapp_handle, launchmode, 1);

	if(launchmode == LAUNCH_HBL)
	{
		strcpy((void*)0xF5E70000,SD_HBL_PATH);
		return 0x01800000;
	}
	else if(launchmode == LAUNCH_FTP)
	{
		strcpy((void*)0xF5E70000,SD_FTP_PATH);
		return 0x01800000;
	}
	else if(launchmode == LAUNCH_SDCAFIINE)
	{
		strcpy((void*)0xF5E70000,SD_SDCAFIINE_PATH);
		return 0x01800000;
	}
	else if(launchmode == LAUNCH_HIDTOVPAD)
	{
		strcpy((void*)0xF5E70000,SD_HIDTOVPAD_PATH);
		return 0x01800000;
	}
	else if(launchmode == LAUNCH_APPSTORE)
	{
		strcpy((void*)0xF5E70000,SD_APPSTORE_PATH);
		return 0x01800000;
	}
	else if(launchmode == LAUNCH_WUP)
	{
		strcpy((void*)0xF5E70000,SD_WUP_PATH);
		return 0x01800000;
	}
	else if(launchmode == LAUNCH_SWAPDRC)
	{
		strcpy((void*)0xF5E70000,SD_SWAPDRC_PATH);
		return 0x01800000;
	}
	else if(launchmode == LAUNCH_TCPGECKO)
	{
		strcpy((void*)0xF5E70000,SD_TCPGECKO_PATH);
		return 0x01800000;
	}
	else if(launchmode == LAUNCH_CUSTOM)
	{
		strcpy((void*)0xF5E70000,SD_CUSTOM_PATH);
		return 0x01800000;
	}
	else if(launchmode == LAUNCH_MOCHA)
	{
		strcpy((void*)0xF5E70000,SD_MOCHA_PATH);
		return 0x01800000;
	}
	else if(launchmode == LAUNCH_VWII_SYSMENU)
	{
		// vwii system menu bootup
		memcpy((void*)0xF5E70000, &VWII_SYSMENU_TID, 8);
		return 0x0180C000;
	}
	else if(launchmode == LAUNCH_VWII_HBC)
	{
		// vwii system menu bootup
		memcpy((void*)0xF5E70000, &VWII_HBC_TID, 8);
		return 0x0180C000;
	}

	//sysmenu or cfw
	if(launchmode == LAUNCH_CFW_IMG)
	{
		OSForceFullRelaunch();
		SYSLaunchMenu();
	}
	else
	{
		if(defaultSlot) //normal menu boot
			SYSLaunchMenu();
		else //show mii select
			_SYSLaunchMenuWithCheckingAccount(slot);
	}
	OSExitThread(0);
	return 0;
}

/* General Input Code */

static unsigned int wpadToVpad(unsigned int buttons)
{
	unsigned int conv_buttons = 0;

	if(buttons & WPAD_BUTTON_LEFT)
		conv_buttons |= VPAD_BUTTON_LEFT;

	if(buttons & WPAD_BUTTON_RIGHT)
		conv_buttons |= VPAD_BUTTON_RIGHT;

	if(buttons & WPAD_BUTTON_DOWN)
		conv_buttons |= VPAD_BUTTON_DOWN;

	if(buttons & WPAD_BUTTON_UP)
		conv_buttons |= VPAD_BUTTON_UP;

	if(buttons & WPAD_BUTTON_PLUS)
		conv_buttons |= VPAD_BUTTON_PLUS;

	if(buttons & WPAD_BUTTON_2)
		conv_buttons |= VPAD_BUTTON_X;

	if(buttons & WPAD_BUTTON_1)
		conv_buttons |= VPAD_BUTTON_Y;

	if(buttons & WPAD_BUTTON_B)
		conv_buttons |= VPAD_BUTTON_B;

	if(buttons & WPAD_BUTTON_A)
		conv_buttons |= VPAD_BUTTON_A;

	if(buttons & WPAD_BUTTON_MINUS)
		conv_buttons |= VPAD_BUTTON_MINUS;

	if(buttons & WPAD_BUTTON_HOME)
		conv_buttons |= VPAD_BUTTON_HOME;

	return conv_buttons;
}

static unsigned int wpadClassicToVpad(unsigned int buttons)
{
	unsigned int conv_buttons = 0;

	if(buttons & WPAD_CLASSIC_BUTTON_LEFT)
		conv_buttons |= VPAD_BUTTON_LEFT;

	if(buttons & WPAD_CLASSIC_BUTTON_RIGHT)
		conv_buttons |= VPAD_BUTTON_RIGHT;

	if(buttons & WPAD_CLASSIC_BUTTON_DOWN)
		conv_buttons |= VPAD_BUTTON_DOWN;

	if(buttons & WPAD_CLASSIC_BUTTON_UP)
		conv_buttons |= VPAD_BUTTON_UP;

	if(buttons & WPAD_CLASSIC_BUTTON_PLUS)
		conv_buttons |= VPAD_BUTTON_PLUS;

	if(buttons & WPAD_CLASSIC_BUTTON_X)
		conv_buttons |= VPAD_BUTTON_X;

	if(buttons & WPAD_CLASSIC_BUTTON_Y)
		conv_buttons |= VPAD_BUTTON_Y;

	if(buttons & WPAD_CLASSIC_BUTTON_B)
		conv_buttons |= VPAD_BUTTON_B;

	if(buttons & WPAD_CLASSIC_BUTTON_A)
		conv_buttons |= VPAD_BUTTON_A;

	if(buttons & WPAD_CLASSIC_BUTTON_MINUS)
		conv_buttons |= VPAD_BUTTON_MINUS;

	if(buttons & WPAD_CLASSIC_BUTTON_HOME)
		conv_buttons |= VPAD_BUTTON_HOME;

	if(buttons & WPAD_CLASSIC_BUTTON_ZR)
		conv_buttons |= VPAD_BUTTON_ZR;

	if(buttons & WPAD_CLASSIC_BUTTON_ZL)
		conv_buttons |= VPAD_BUTTON_ZL;

	if(buttons & WPAD_CLASSIC_BUTTON_R)
		conv_buttons |= VPAD_BUTTON_R;

	if(buttons & WPAD_CLASSIC_BUTTON_L)
		conv_buttons |= VPAD_BUTTON_L;

	return conv_buttons;
}

static unsigned int getButtonsDown(unsigned int padscore_handle, unsigned int vpad_handle)
{
	int(*WPADProbe)(int chan, int * pad_type);
	int(*KPADRead)(int chan, void * data, int size);
	OSDynLoad_FindExport(padscore_handle, 0, "WPADProbe",&WPADProbe);
	OSDynLoad_FindExport(padscore_handle, 0, "KPADRead",&KPADRead);

	unsigned int btnDown = 0;

	int(*VPADRead)(int controller, VPADData *buffer, unsigned int num, int *error);
	OSDynLoad_FindExport(vpad_handle, 0, "VPADRead", &VPADRead);

	int vpadError = -1;
	VPADData vpad;
	VPADRead(0, &vpad, 1, &vpadError);
	if(vpadError == 0)
		btnDown |= vpad.btns_d;

	int i;
	for(i = 0; i < 4; i++)
	{
		int controller_type;
		if(WPADProbe(i, &controller_type) != 0)
			continue;
		KPADData kpadData;
		KPADRead(i, &kpadData, 1);
		if(kpadData.device_type <= 1)
			btnDown |= wpadToVpad(kpadData.btns_d);
		else
			btnDown |= wpadClassicToVpad(kpadData.classic.btns_d);
	}

	return btnDown;
}
