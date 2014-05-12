/*************************************************************************************************
-- DVChange - Digital Vibrance Command line Change
-- Usage: DVChange <value 0-63>
-- Ex: DVChange 63 (For Digital Vibrance 100 %)
-- Ex: DVChange 0 (For Digital Vibrance 50%)
--
-- Author: Chad Fisher
-- Reference Sites:
--	 http://stackoverflow.com/questions/13291783/how-to-get-the-id-memory-address-of-dll-function
--	 http://evolution3d.googlecode.com/svn/trunk/Plugin/NvRenderer/NvApi/nvapi.h
--	 http://eliang.blogspot.com/2011/05/getting-nvidia-gpu-usage-in-c.html
--	 http://searchcode.com/codesearch/view/37402245
--	 http://stackoverflow.com/questions/6165628/use-python-ctypes-to-interface-with-nvapi-follow-up-with-demonstration-code
**************************************************************************************************/

#include <iostream>
#include <Windows.h>
#include "nvapi.h"

using namespace std;

//Struct used for getting Digital Vibrance Information via NVAPI_GetDVCInfo()
typedef struct
{
	NvU32   version;            //IN version info  
	NvU32   currentLevel;       //OUT current DVC level
	NvU32   minLevel;           //OUT min range level
	NvU32   maxLevel;           //OUT max range level
} NV_DISPLAY_DVC_INFO;

#define NV_DISPLAY_DVC_INFO_VER  MAKE_NVAPI_VERSION(NV_DISPLAY_DVC_INFO,1)

//NVAPI_INTERFACE NvAPI_GetDVCInfo(NvDisplayHandle hNvDisplay, NvU32 outputId, NV_DISPLAY_DVC_INFO *pDVCInfo);
typedef NvAPI_Status (* GetDVCInfo_t)(NvDisplayHandle hNvDisplay, NvU32 outputId, NV_DISPLAY_DVC_INFO *pDVCInfo);

//NVAPI_INTERFACE NvAPI_SetDVCLevel(NvDisplayHandle hNvDisplay, NvU32 outputId, NvU32 level);
typedef NvAPI_Status (* SetDVCInfo_t)(NvDisplayHandle hNvDisplay, NvU32 outputId, NvU32 level);

//NVAPI_INTERFACE NvAPI_EnumPhysicalGPUs(NvPhysicalGpuHandle nvGPUHandle[NVAPI_MAX_PHYSICAL_GPUS], NvU32 *pGpuCount);
typedef NvAPI_Status (* EnumPhysicalGPUs_t)(NvPhysicalGpuHandle nvGPUHandle[NVAPI_MAX_PHYSICAL_GPUS], NvU32 *pGpuCount);

//typedef int *(*NvAPI_QueryInterface_t)(unsigned int offset);
typedef int *(* NvAPI_QueryInterface_t)(unsigned int offset);

//Checks the error message
void CheckError()
{
	LPVOID lpMsgBuf;
	DWORD dw = GetLastError();

	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		dw,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf,
		0, NULL);

	wprintf(L"Error: %s\n", lpMsgBuf);
}

// Enumerate display handles
void GetDisplays(NvDisplayHandle *hDisplay_a, NvU32 &nvDisplayCount)
{
	NvAPI_Status status = NVAPI_OK;
	nvDisplayCount = 0;
	for (unsigned int i = 0; status == NVAPI_OK; i++)
	{
		status = NvAPI_EnumNvidiaDisplayHandle(i, &hDisplay_a[i]);

		if (status == NVAPI_OK)
		{
			nvDisplayCount++;
		}
	}
}

//Enumerate GPU handles
void GetGPUS(NvPhysicalGpuHandle *hGPU_a, NvU32 &gpuCount)
{
	NvAPI_Status status = NVAPI_OK;
	status = NvAPI_EnumPhysicalGPUs(hGPU_a, &gpuCount);
	if (status != NVAPI_OK)
	{
		printf("NvAPI_EnumPhysicalGPUs() failed with status %d\n", status);
		printf("\n");
	}
}

int main(int argc, char** argv)
{
	if (argc != 2)
	{
		printf("Usage: DVChange <value 0-63>\n");
		printf("Ex (Setting Digital Vibrance to 100%): DVChange 63\n");
		return EXIT_FAILURE;
	}

	//NvAPI_Initialize();
	HINSTANCE hLib = LoadLibrary(L"nvapi.dll");

	//Get DLL Library - nvapi.dll
	if(hLib == NULL)
	{
		printf("Error: Couldn't LoadLibrary - nvapi.dll\n");
		CheckError();
		return EXIT_FAILURE;
	}
	printf("Loaded nvapi.dll ...\n\n");

	//Get Functions from nvapi.dll
	GetDVCInfo_t GetDVCInfo = NULL;
	SetDVCInfo_t SetDVCInfo = NULL;


	NvAPI_QueryInterface_t NvAPI_QueryInterface = (NvAPI_QueryInterface_t)GetProcAddress(hLib, "nvapi_QueryInterface");
	GetDVCInfo = (GetDVCInfo_t)(*NvAPI_QueryInterface)(0x4085DE45);
	SetDVCInfo = (SetDVCInfo_t)(*NvAPI_QueryInterface)(0x172409B4);

	//Check to ensure we have retrieved functions correctly
	if (GetDVCInfo == NULL || SetDVCInfo == NULL)
	{
		printf("Error: Couldn't retrieve functions correctly\n");
		CheckError();
		return EXIT_FAILURE;
	}

	NvAPI_Initialize();

	//Get Display Handles / Print Displays
	NvDisplayHandle hDisplay_a[NVAPI_MAX_PHYSICAL_GPUS * NVAPI_MAX_DISPLAY_HEADS] = { 0 };
	NvU32 nvDisplayCount = 0;
	GetDisplays(hDisplay_a, nvDisplayCount);
	printf("\nDisplays: ");
	for (unsigned int i = 0; i < nvDisplayCount; i++)
		printf("\nDisplay #%d: %p", i, (void*)hDisplay_a[i]);
	printf("\n");


	//Get GPU Handles
	NvPhysicalGpuHandle hGPU_a[NVAPI_MAX_PHYSICAL_GPUS] = { 0 }; // handle to GPUs
	NvU32 gpuCount = 0;
	GetGPUS(hGPU_a, gpuCount);
	printf("\nGPU Count:\n");
	printf("Total number of GPU's = %u\n", gpuCount);


	//Check DVC Settings / Set DVC Value
	NV_DISPLAY_DVC_INFO dvcInfo;
	NvU32 dvcValue = atoi(argv[1]);
	dvcInfo.version = NV_DISPLAY_DVC_INFO_VER;

	if (dvcValue > 63) { dvcValue = 63; }
	if (dvcValue < 0) { dvcValue = 0; }

	for (unsigned int i = 0; i < nvDisplayCount; i++)
	{
		(*SetDVCInfo)(hDisplay_a[i], NULL, dvcValue);
		(*GetDVCInfo)(hDisplay_a[i], NULL, &dvcInfo);
		printf("\nDisplay #%d: %p", i, (void*)hDisplay_a[i]);
		printf("\n  -DVC Value: %u", dvcInfo.currentLevel);
	}

	FreeLibrary(hLib);
	printf("\n");
	return EXIT_SUCCESS;
}