// DeletePrintDemo.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <guiddef.h> 
#include <windows.h>
#include <setupapi.h>
#include <vector>
#include <iostream>
#include <strsafe.h>
#include <shlwapi.h>
#include <devguid.h>    // for GUID_DEVCLASS_CDROM etc
#include <cfgmgr32.h>   // for MAX_DEVICE_ID_LEN, CM_Get_Parent and CM_Get_Device_ID
#define INITGUID
#include <tchar.h>
#include <stdio.h>
using namespace std;

#ifdef DEFINE_DEVPROPKEY
#undef DEFINE_DEVPROPKEY
#endif
#ifdef INITGUID
#define DEFINE_DEVPROPKEY(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8, pid) EXTERN_C const DEVPROPKEY DECLSPEC_SELECTANY name = { { l, w1, w2, { b1, b2,  b3,  b4,  b5,  b6,  b7,  b8 } }, pid }
#else
#define DEFINE_DEVPROPKEY(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8, pid) EXTERN_C const DEVPROPKEY name
#endif // INITGUID

// include DEVPKEY_Device_BusReportedDeviceDesc from WinDDK\7600.16385.1\inc\api\devpkey.h
DEFINE_DEVPROPKEY(DEVPKEY_Device_BusReportedDeviceDesc, 0x540b947e, 0x8b40, 0x45bc, 0xa8, 0xa2, 0x6a, 0x0b, 0x89, 0x4c, 0xbd, 0xa2, 4);     // DEVPROP_TYPE_STRING
DEFINE_DEVPROPKEY(DEVPKEY_Device_ContainerId, 0x8c7ed206, 0x3f8a, 0x4827, 0xb3, 0xab, 0xae, 0x9e, 0x1f, 0xae, 0xfc, 0x6c, 2);     // DEVPROP_TYPE_GUID
DEFINE_DEVPROPKEY(DEVPKEY_Device_FriendlyName, 0xa45c254e, 0xdf1c, 0x4efd, 0x80, 0x20, 0x67, 0xd1, 0x46, 0xa8, 0x50, 0xe0, 14);    // DEVPROP_TYPE_STRING
DEFINE_DEVPROPKEY(DEVPKEY_DeviceDisplay_Category, 0x78c34fc8, 0x104a, 0x4aca, 0x9e, 0xa4, 0x52, 0x4d, 0x52, 0x99, 0x6e, 0x57, 0x5a);  // DEVPROP_TYPE_STRING_LIST
DEFINE_DEVPROPKEY(DEVPKEY_Device_LocationInfo, 0xa45c254e, 0xdf1c, 0x4efd, 0x80, 0x20, 0x67, 0xd1, 0x46, 0xa8, 0x50, 0xe0, 15);    // DEVPROP_TYPE_STRING
DEFINE_DEVPROPKEY(DEVPKEY_Device_Manufacturer, 0xa45c254e, 0xdf1c, 0x4efd, 0x80, 0x20, 0x67, 0xd1, 0x46, 0xa8, 0x50, 0xe0, 13);    // DEVPROP_TYPE_STRING
DEFINE_DEVPROPKEY(DEVPKEY_Device_SecuritySDS, 0xa45c254e, 0xdf1c, 0x4efd, 0x80, 0x20, 0x67, 0xd1, 0x46, 0xa8, 0x50, 0xe0, 26);    // DEVPROP_TYPE_SECURITY_DESCRIPTOR_STRING
DEFINE_DEVPROPKEY(DEVPKEY_NAME,                          0xb725f130, 0x47ef, 0x101a, 0xa5, 0xf1, 0x02, 0x60, 0x8c, 0x9e, 0xeb, 0xac, 10);    // DEVPROP_TYPE_STRING
DEFINE_DEVPROPKEY(DEVPKEY_Device_DeviceDesc,             0xa45c254e, 0xdf1c, 0x4efd, 0x80, 0x20, 0x67, 0xd1, 0x46, 0xa8, 0x50, 0xe0, 2);     // DEVPROP_TYPE_STRING
DEFINE_DEVPROPKEY(DEVPKEY_Device_HardwareIds,            0xa45c254e, 0xdf1c, 0x4efd, 0x80, 0x20, 0x67, 0xd1, 0x46, 0xa8, 0x50, 0xe0, 3);

#define ARRAY_SIZE(arr)     (sizeof(arr)/sizeof(arr[0]))
#pragma comment(lib,"setupapi.lib")
typedef BOOL(WINAPI *FN_SetupDiGetDevicePropertyW)(
	__in       HDEVINFO DeviceInfoSet,
	__in       PSP_DEVINFO_DATA DeviceInfoData,
	__in       const DEVPROPKEY *PropertyKey,
	__out      DEVPROPTYPE *PropertyType,
	__out_opt  PBYTE PropertyBuffer,
	__in       DWORD PropertyBufferSize,
	__out_opt  PDWORD RequiredSize,
	__in       DWORD Flags
	);
FN_SetupDiGetDevicePropertyW fn_SetupDiGetDevicePropertyW = (FN_SetupDiGetDevicePropertyW)
	GetProcAddress(GetModuleHandle(TEXT("Setupapi.dll")), "SetupDiGetDevicePropertyW");
//U盘 interface class GUID 
GUID IID_CLASS_WCEUSBS ={0xa5dcbf10, 0x6530, 0x11d2, 0x90, 0x1f, 0x00, 0xc0, 0x4f, 0xb9, 0x51, 0xed};
GUID GUID_DEVINTERFACE_USB_DEVICE = {0xA5DCBF10L, 0x6530, 0x11D2, 0x90, 0x1F, 0x00, 0xC0, 0x4F, 0xB9, 0x51, 0xED};
GUID GUID_DEVINTERFACE_USB_DEVICE2 = {0x4d36e979,  0xe325, 0x11ce, 0xbf, 0xc1, 0x08, 0x00, 0x2b, 0xe1, 0x03, 0x18};
// 打印机信息结构体
typedef struct _PCS_PRINT_INFO
{
	DWORD  dwPrintIndex;                    // 打印机在数组中的索引值
	WCHAR  wcPrintName[MAX_PATH];           // 打印机的名称，网络打印机名称是参照windows的用IP和共享名称组合的
	WCHAR  wcPrintModel[MAX_PATH];          // 打印机的型号
	WCHAR  wcTruethPrintName[MAX_PATH];     // 真实的打印机名称，用户获取打印机句柄
	WCHAR  wcShareIp[MAX_PATH];             // 共享打印机的IP,如果时本地共享打印机，则此IP是本地IP
	WCHAR  wcShareName[MAX_PATH];           // 共享打印机共享出来的名称
	WCHAR  wcPortName[MAX_PATH];            // 打印机的端口名
	DWORD  dwPrintType;                     // 打印机的类型，参考PCSPRINTTYPE
	BOOL   bIsDefaultPrint;                 // 是否为默认打印机
}PCS_PRINT_INFO, *PPCS_PRINT_INFO;

BOOL FindSon(DEVINST devInst,TCHAR* device_name)
{
	BOOL ret = FALSE;
	DEVINST devinstNext = 0;
	ULONG len = 4096;
	WCHAR szBuffer[4096];
	CM_Get_DevNode_Registry_Property(devInst, 
		CM_DRP_HARDWAREID, 
		NULL, 
		szBuffer, 
		&len, 0);
	printf("##buff = %ls\n",szBuffer);
	if (_tcsicmp(szBuffer , device_name) == 0)
	{
		printf("####find same to device_name!#### %s\n",szBuffer);
		return TRUE;
	}
	else
	{
		printf("####no match! %ls\n",device_name);
	}
	CONFIGRET cr = CR_SUCCESS;
	cr = CM_Get_Child(&devinstNext,devInst,NULL);
	
	if (cr == CR_SUCCESS)
	{
		ret = FindSon(devinstNext,device_name);
	}
	if (ret)
	{
		return ret;
	}
	cr = CM_Get_Sibling(&devinstNext,devInst,NULL);
	if (cr == CR_SUCCESS)
	{
		ret = FindSon(devinstNext,device_name);
	}
	return ret;
}
void RemoveDevice(TCHAR* device_name)
{
	BOOL bRes = FALSE;
	LPGUID pInterfaceGuid = &GUID_DEVINTERFACE_USB_DEVICE;
	SP_DEVINFO_DATA		devinfo_data = {0};

	SP_DEVICE_INTERFACE_DATA			inf_data = {0};
	PSP_DEVICE_INTERFACE_DETAIL_DATA_W	pinf_detail_data = NULL;
	unsigned long						inf_detail_size = 0;

	HDEVINFO hDeviceInfo = SetupDiGetClassDevs( pInterfaceGuid,
		NULL,
		NULL,
		DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);

	if (INVALID_HANDLE_VALUE == hDeviceInfo)
	{
		return;
	}

	SP_DEVICE_INTERFACE_DATA spDevInterData;		

	memset(&spDevInterData, 0x00, sizeof(SP_DEVICE_INTERFACE_DATA));
	spDevInterData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

	DWORD dwIndex = 0;
	
	while (TRUE)
	{
		ZeroMemory(&devinfo_data, sizeof(devinfo_data));
		devinfo_data.cbSize = sizeof(devinfo_data);
		if (!SetupDiEnumDeviceInfo(hDeviceInfo, dwIndex, &devinfo_data))
		{
			if (GetLastError() == ERROR_NO_MORE_ITEMS)
			{
				printf("SetupDiEnumDeviceInfo fail err =%d\n",GetLastError());
				break;
			}
			printf("SetupDiEnumDeviceInfo fail err =%d\n",GetLastError());
			continue;
		}

		TCHAR buf[MAX_PATH];
		DWORD nSize = 0;
		DWORD DataT;
		ULONG len = 4096;
		WCHAR szBuffer[4096];
		CONFIGRET cr = CR_SUCCESS;
		DEVINST oriDevinst = devinfo_data.DevInst;
		/*inf_data.cbSize = sizeof(inf_data);
		
		if ( !SetupDiEnumDeviceInterfaces(hDeviceInfo, &devinfo_data, &GUID_DEVINTERFACE_USB_DEVICE, dwIndex, &inf_data) )
		{
			printf("SetupDiEnumDeviceInterfaces fail err =%d\n",GetLastError());
			break;
		}
		SetupDiGetDeviceInterfaceDetailW(hDeviceInfo, &inf_data, NULL, 0, &inf_detail_size, NULL);*/
		int i = 0;
		if (FindSon(devinfo_data.DevInst,device_name))
		{
			printf("####find same to regedit!#### %s\n",szBuffer);
			if( SetupDiRemoveDevice(hDeviceInfo, &devinfo_data) )
			{
				printf("success %d\n",GetLastError());
				break;
			}
			else
			{
				printf("failZZZZZ%d\n",GetLastError());
			}
		}

		//while(cr == CR_SUCCESS)
		//{
		//	++i;
		//	//cr = CM_Get_DevNode_Registry_Property(devinfo_data.DevInst, 
		//	//	CM_DRP_HARDWAREID, 
		//	//	NULL, 
		//	//	szBuffer, 
		//	//	&len, 0);
		//	DEVINST devinstNext = 0;
		//	cr = CM_Get_Child(&devinstNext,devinfo_data.DevInst,NULL);
		//	devinfo_data.DevInst = devinstNext;

		//	CM_Get_DevNode_Registry_Property(devinfo_data.DevInst, 
		//		CM_DRP_HARDWAREID, 
		//		NULL, 
		//		szBuffer, 
		//		&len, 0);
		//	printf("###%d:buff = %ls  cr = %d\n",i,szBuffer,cr);
		//	//cr = CM_Get_Child(&devinfo_data.DevInst,devinfo_data.DevInst,NULL);


		//	WCHAR szName[4096];
		//	if (fn_SetupDiGetDevicePropertyW && fn_SetupDiGetDevicePropertyW(hDeviceInfo, &devinfo_data, &DEVPKEY_NAME,
		//	&DataT, (BYTE*)szName, sizeof(szName), &nSize, 0)) 
		//	{
		//		if (fn_SetupDiGetDevicePropertyW(hDeviceInfo, &devinfo_data, &DEVPKEY_NAME,
		//			&DataT, (BYTE*)szName, sizeof(szName), &nSize, 0))
		//		{
		//			printf("Bus Reported Device Description: \"%ls\"\n", szName);
		//		}
		//	}
		//	
		//	if (_tcsicmp(szBuffer , device_name) == 0)
		//	{
		//		printf("####find same to regedit!#### %s\n",szBuffer);
		//		if( SetupDiRemoveDevice(hDeviceInfo, &devinfo_data) )
		//		{
		//			printf("success %d\n",GetLastError());
		//			break;
		//		}
		//		else
		//		{
		//			printf("failZZZZZ%d\n",GetLastError());
		//		}
		//	}
		//}
		dwIndex++;
	}
	if (!SetupDiDestroyDeviceInfoList(hDeviceInfo))
	{
		OutputDebugStringW(L"SetupDiDestroyDeviceInfoList Error");
	}
}

BOOL SearchDevice(vector<wstring> &vDevicePath)
{
	BOOL bRes = FALSE;
	LPGUID pInterfaceGuid = &GUID_DEVINTERFACE_USB_DEVICE;
	SP_DEVINFO_DATA		devinfo_data = {0};

	HDEVINFO hDeviceInfo = SetupDiGetClassDevs( pInterfaceGuid,
		NULL,
		NULL,
		DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);

	if (INVALID_HANDLE_VALUE == hDeviceInfo)
	{
		goto Exit;
	}

	SP_DEVICE_INTERFACE_DATA spDevInterData;		

	memset(&spDevInterData, 0x00, sizeof(SP_DEVICE_INTERFACE_DATA));
	spDevInterData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

	DWORD dwIndex = 0;
	
	while (TRUE)
	{
		ZeroMemory(&devinfo_data, sizeof(devinfo_data));
		devinfo_data.cbSize = sizeof(devinfo_data);
		if (!SetupDiEnumDeviceInfo(hDeviceInfo, dwIndex, &devinfo_data))
		{
			if (GetLastError() == ERROR_NO_MORE_ITEMS)
			{
				printf("SetupDiEnumDeviceInfo fail err =%d\n",GetLastError());
				break;
			}
			printf("SetupDiEnumDeviceInfo fail err =%d\n",GetLastError());
			continue;
		}

		TCHAR buf[MAX_PATH];
		DWORD nSize = 0;
		DWORD DataT;
		/*if (!SetupDiGetDeviceInstanceId(hDeviceInfo,&devinfo_data,buf,sizeof(buf),&nSize))
		{

		}
		else
		{*/
			WCHAR szBuffer[4096];
			if (fn_SetupDiGetDevicePropertyW && fn_SetupDiGetDevicePropertyW(hDeviceInfo, &devinfo_data, &DEVPKEY_Device_BusReportedDeviceDesc,
				&DataT, (BYTE*)szBuffer, sizeof(szBuffer), &nSize, 0)) {

					if (fn_SetupDiGetDevicePropertyW(hDeviceInfo, &devinfo_data, &DEVPKEY_Device_BusReportedDeviceDesc,
						&DataT, (BYTE*)szBuffer, sizeof(szBuffer), &nSize, 0))
						_tprintf(TEXT("    Bus Reported Device Description: \"%ls\"\n"), szBuffer);
			}
			
		//}
		

		if (!SetupDiEnumDeviceInterfaces( hDeviceInfo,
			&devinfo_data,
			pInterfaceGuid,
			dwIndex,
			&spDevInterData))

		{
			if (ERROR_NO_MORE_ITEMS == GetLastError())
			{
				printf("No more interface fail err = %d\n",GetLastError());
			}
			else
			{
				printf("SetupDiEnumDeviceInterfaces Error fail err =%d\n",GetLastError());
			}

			goto Exit;
		}

		DWORD dwRequiredLength = 0;		

		if (!SetupDiGetDeviceInterfaceDetail( hDeviceInfo,
			&spDevInterData,
			NULL, 
			0,
			&dwRequiredLength,
			NULL))
		{
			if (ERROR_INSUFFICIENT_BUFFER != GetLastError())
			{
				printf("calculate require length fail err =%d\n",GetLastError());
				//goto Exit;
			}
		}
	
		PSP_DEVICE_INTERFACE_DETAIL_DATA pSpDIDetailData;		

		pSpDIDetailData = NULL;
		pSpDIDetailData = (PSP_DEVICE_INTERFACE_DETAIL_DATA)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwRequiredLength);

		if(NULL == pSpDIDetailData)
		{
			OutputDebugStringW(L"HeapAlloc Memory Failed");

			if (!SetupDiDestroyDeviceInfoList(hDeviceInfo))
			{
				OutputDebugStringW(L"SetupDiDestroyDeviceInfoList Error");
			}

			goto Exit;
		}

		pSpDIDetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

		if (!SetupDiGetDeviceInterfaceDetail( hDeviceInfo,
			&spDevInterData,
			pSpDIDetailData,
			dwRequiredLength,
			&dwRequiredLength,
			NULL))
		{
			OutputDebugStringW(L"SetupDiGetDeviceInterfaceDetail Error");
			goto Exit;
		}
		char				serial_number[1024] = {0};
		wstring wcsDevicePath = pSpDIDetailData->DevicePath;

		vDevicePath.push_back(wcsDevicePath); 

		/*if( SetupDiRemoveDevice(hDeviceInfo, &devinfo_data) )
		{
		printf("success %d\n",GetLastError());
		break;
		}
		else
		{
		printf("failZZZZZ%d\n",GetLastError());
		}*/

		if (NULL != pSpDIDetailData)
		{
			HeapFree(GetProcessHeap(), 0, pSpDIDetailData);
			pSpDIDetailData = NULL;
		}
		dwIndex++;
	}

	if (!SetupDiDestroyDeviceInfoList(hDeviceInfo))
	{
		OutputDebugStringW(L"SetupDiDestroyDeviceInfoList Error");
	}

	bRes = TRUE;

Exit:
	return bRes;
}

HANDLE GetPrintHandle(WCHAR *pPrintName, DWORD dwDesiredAccess = PRINTER_ALL_ACCESS)
{
	if (NULL == pPrintName)
	{
		return NULL;
	}

	HANDLE  hPrinter = NULL;
	PRINTER_DEFAULTS pd;
	ZeroMemory(&pd, sizeof(pd));
	pd.DesiredAccess = dwDesiredAccess;
	if (!::OpenPrinter(pPrintName, &hPrinter, &pd))
	{
		return NULL;
	}
	return hPrinter;
}

void GetPrint()
{
	DWORD dwNeeded = 0;
	DWORD dwReturned = 0;
	TCHAR wcDefaultPrint[MAX_PATH] = {0};
	DWORD dwDefaultPrintLen = MAX_PATH;
	PCS_PRINT_INFO sPrintInfo = {0};
	
	// 获取本地打印机的个数 
	(void)EnumPrinters(PRINTER_ENUM_LOCAL|PRINTER_ENUM_CONNECTIONS, NULL, 
		2, NULL, 0, &dwNeeded, &dwReturned);
	LPBYTE lpBuffer = new(std::nothrow) BYTE[dwNeeded];
	if (!lpBuffer )
	{

	}

	ZeroMemory(lpBuffer,dwNeeded);
	if (!EnumPrinters(PRINTER_ENUM_LOCAL|PRINTER_ENUM_CONNECTIONS,NULL, 
		2, lpBuffer, dwNeeded, &dwNeeded, &dwReturned))
	{	
	
	}
	PRINTER_INFO_2* pPrintInfo2 = (PRINTER_INFO_2*)lpBuffer;
	printf("dwReturn = %d\n",dwReturned);
	HANDLE  hPrinter = NULL;
	SP_DEVINFO_DATA devinfo_data = {0};
	ZeroMemory(&devinfo_data,sizeof(devinfo_data));
	devinfo_data.cbSize = sizeof(devinfo_data);
	for (UINT nIndex = 0; nIndex < dwReturned; nIndex++)
	{
		printf("%ls\n",pPrintInfo2->pPrinterName);
		//if (_tcsicmp(pPrintInfo2->pPrinterName,L"EPSON WF-M1030 Series") == 0)
		if (TRUE)
		{
			WCHAR wcHardIdKey[MAX_PATH] = {0};  // HardId路径
			DWORD dwType = REG_SZ;
			WCHAR wszHardId[MAX_PATH] = { 0 };
			DWORD dwDataLen = sizeof(wszHardId);
			WCHAR szSubKey[MAX_PATH] = L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Print";
			HKEY key = NULL;
			//_snwprintf_s(wcHardIdKey, _countof(wcHardIdKey)-1, _TRUNCATE, _T("%s\\Printers\\%s\\PnPData"), szSubKey,pPrintInfo2->pPrinterName);
			_snwprintf_s(wcHardIdKey, _countof(wcHardIdKey)-1, _TRUNCATE, _T("%s\\Printers\\Canon G3000 series Printer\\PnPData"), szSubKey);
			
			if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, wcHardIdKey,
				0L,  KEY_READ, &key) == ERROR_SUCCESS) {
					if (RegQueryValueEx(key, L"HardwareID", NULL, &dwType, (LPBYTE)wszHardId, &dwDataLen) ==
						ERROR_SUCCESS) {
							//key_found = true;
					}
					RegCloseKey(key);
			}
			else
			{
				printf("err = %d\n",RegOpenKeyEx(HKEY_LOCAL_MACHINE, wcHardIdKey,0L, KEY_QUERY_VALUE, &key));
			}

			RemoveDevice(wszHardId);
			break;
		}
		
		pPrintInfo2++;
	}
}

void GetDevicePropertiesCfgmgr32()
{
	CONFIGRET cr = CR_SUCCESS;
	PWSTR DeviceList = NULL;
	ULONG DeviceListLength = 0;
	PWSTR CurrentDevice;
	DEVINST Devinst;
	WCHAR DeviceDesc[2048];
	DEVPROPTYPE PropertyType;
	ULONG PropertySize;
	DWORD Index = 0;

	cr = CM_Get_Device_ID_List_Size(&DeviceListLength,
		NULL,
		CM_GETIDLIST_FILTER_PRESENT);

	if (cr != CR_SUCCESS)
	{
		goto Exit;
	}

	DeviceList = (PWSTR)HeapAlloc(GetProcessHeap(),
		HEAP_ZERO_MEMORY,
		DeviceListLength * sizeof(WCHAR));

	if (DeviceList == NULL) {
		goto Exit;
	}

	cr = CM_Get_Device_ID_List(L"{4d36e979-e325-11ce-bfc1-08002be10318}",
		DeviceList,
		16384,
		CM_GETIDLIST_FILTER_CLASS);

	if (cr != CR_SUCCESS)
	{
		goto Exit;
	}

	for (CurrentDevice = DeviceList;
		*CurrentDevice;
		CurrentDevice += wcslen(CurrentDevice) + 1)
	{

		// If the list of devices also includes non-present devices,
		// CM_LOCATE_DEVNODE_PHANTOM should be used in place of
		// CM_LOCATE_DEVNODE_NORMAL.
		cr = CM_Locate_DevNode(&Devinst,
			CurrentDevice,
			CM_LOCATE_DEVNODE_NORMAL);

		if (cr != CR_SUCCESS)
		{
			goto Exit;
		}

		// Query a property on the device.  For example, the device description.
		PropertySize = sizeof(DeviceDesc);
		/*cr = CM_Get_DevNode_Property(Devinst,
			&DEVPKEY_Device_DeviceDesc,
			&PropertyType,
			(PBYTE)DeviceDesc,
			&PropertySize,
			0);
*/
		if (cr != CR_SUCCESS)
		{
			Index++;
			continue;
		}

		/*if (PropertyType != DEVPROP_TYPE_STRING)
		{
			Index++;
			continue;
		}*/

		Index++;
	}

Exit:

	if (DeviceList != NULL)
	{
		HeapFree(GetProcessHeap(),
			0,
			DeviceList);
	}

	return;
}

int _tmain(int argc, _TCHAR* argv[])
{
	GetPrint();
	/*vector<wstring> vDevicePath;
	SearchDevice(vDevicePath);

	vector<wstring>::iterator iter;

	for (iter = vDevicePath.begin(); iter != vDevicePath.end(); ++iter)
	{
		wcout << (*iter).c_str() << endl;
	}*/
	system("pause");
	return 0;
}

