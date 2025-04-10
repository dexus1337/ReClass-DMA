#include "vmmdll.h"
#include "leechcore.h"
#include "ReClassNET_Plugin.hpp"
#include <algorithm>
#include <cstdint>
#include <vector>
#include <filesystem>

static VMM_HANDLE _hVmm = NULL;

static const bool _hasMemMap = std::filesystem::exists("mmap.txt");

extern "C" RC_Export void RC_CallConv EnumerateProcesses(EnumerateProcessCallback callbackProcess) {
	if (callbackProcess == nullptr) {
		return;
	}

	if (!_hVmm) {
		if (_hasMemMap)
		{
			LPCSTR argv[] = { "-v", "-device", "fpga", "-memmap", "mmap.txt", "-waitinitialize" };
			_hVmm = VMMDLL_Initialize(6, argv);
		}
		else
		{
			LPCSTR argv[] = { "-v", "-device", "fpga", "-waitinitialize" };
			_hVmm = VMMDLL_Initialize(4, argv);
		}

		if (!_hVmm) {
			printf( "FAIL: VMMDLL_Initialize\n");

			exit(-1);
		}
	}

	BOOL result;
	SIZE_T cPIDs = 0;
	DWORD i, * pPIDs = NULL;

	result =
		VMMDLL_PidList(_hVmm, NULL, &cPIDs) && (pPIDs = (DWORD*)malloc((SIZE_T)(cPIDs * sizeof(DWORD)))) && VMMDLL_PidList(_hVmm, pPIDs, &cPIDs);

	if (!result) {
		free(pPIDs);
		return;
	}

    EnumerateProcessData dataphy = {};
    dataphy.Id = -1;
    MultiByteToUnicode("Physical Memory", dataphy.Name, PATH_MAXIMUM_LENGTH);
    callbackProcess(&dataphy);

	for (i = 0; i < cPIDs; i++) {
		DWORD dwPID = pPIDs[i];

		VMMDLL_PROCESS_INFORMATION info;
		SIZE_T cbInfo = sizeof(VMMDLL_PROCESS_INFORMATION);
		memset(&info, 0, cbInfo);
		info.magic = VMMDLL_PROCESS_INFORMATION_MAGIC;
		info.wVersion = VMMDLL_PROCESS_INFORMATION_VERSION;

		result = VMMDLL_ProcessGetInformation(_hVmm, dwPID, &info, &cbInfo);

		if (result) {
			EnumerateProcessData data = {};
			data.Id = dwPID;
			MultiByteToUnicode(info.szNameLong, data.Name, PATH_MAXIMUM_LENGTH);

			LPSTR szPathUser = VMMDLL_ProcessGetInformationString(_hVmm, dwPID, VMMDLL_PROCESS_INFORMATION_OPT_STRING_PATH_USER_IMAGE);

			if (szPathUser) {
				MultiByteToUnicode(szPathUser, data.Path, PATH_MAXIMUM_LENGTH);
			}

			callbackProcess(&data);

            data.Id += 10000000;

            memset(data.Name, 0, sizeof(data.Name));

			MultiByteToUnicode("[WRITE ACCESS]", data.Name, PATH_MAXIMUM_LENGTH);
            *(data.Name + 14) = L' ';
			MultiByteToUnicode(info.szNameLong, data.Name + 15, PATH_MAXIMUM_LENGTH);

			callbackProcess(&data);
		}
	}

	free(pPIDs);
}

extern "C" RC_Export void RC_CallConv EnumerateRemoteSectionsAndModules(RC_Pointer handle, EnumerateRemoteSectionsCallback callbackSection,
	EnumerateRemoteModulesCallback callbackModule) {
	if (callbackSection == nullptr && callbackModule == nullptr) {
		return;
	}

	BOOL result;
    
    int64_t pid = reinterpret_cast< int64_t >(handle);

    if (pid > 10000000)
        pid -= 10000000;
        
	DWORD dwPID = static_cast< DWORD >( pid );
	ULONG64 i, j;

	DWORD cMemMapEntries = 0;
	PVMMDLL_MAP_PTE pMemMapEntries = NULL;
	PVMMDLL_MAP_PTEENTRY memMapEntry = NULL;

    VMMDLL_PROCESS_INFORMATION info;
    SIZE_T cbInfo = sizeof(VMMDLL_PROCESS_INFORMATION);
    memset(&info, 0, cbInfo);
    info.magic = VMMDLL_PROCESS_INFORMATION_MAGIC;
    info.wVersion = VMMDLL_PROCESS_INFORMATION_VERSION;

    result = VMMDLL_ProcessGetInformation(_hVmm, dwPID, &info, &cbInfo);

    if (result) 
    {
        EnumerateRemoteSectionData section = {};
        section.BaseAddress = reinterpret_cast<RC_Pointer>(info.win.vaEPROCESS);
        section.Size = 8;

        section.Protection = SectionProtection::Read;
        section.Category = SectionCategory::DATA;
        section.Type = SectionType::Private;

        MultiByteToUnicode("EPROCESS", section.Name, PATH_MAXIMUM_LENGTH);

        callbackSection(&section);

        section = {};
        section.BaseAddress = reinterpret_cast<RC_Pointer>(info.paDTB);
        section.Size = 8;

        section.Protection = SectionProtection::Read;
        section.Category = SectionCategory::DATA;
        section.Type = SectionType::Private;

        MultiByteToUnicode("CR3", section.Name, PATH_MAXIMUM_LENGTH);

        callbackSection(&section);
    }

	result = VMMDLL_Map_GetPteU(_hVmm, dwPID, true, &pMemMapEntries);

	if (result) 
    {

        std::vector< EnumerateRemoteSectionData > sections;

        for (i = 0; i < pMemMapEntries->cMap; i++) {
            memMapEntry = &pMemMapEntries->pMap[i];

            EnumerateRemoteSectionData section = {};
            section.BaseAddress = (RC_Pointer)memMapEntry->vaBase;
            section.Size = memMapEntry->cPages << 12;

            section.Protection = SectionProtection::NoAccess;
            section.Category = SectionCategory::Unknown;

            if (memMapEntry->fPage & VMMDLL_MEMMAP_FLAG_PAGE_NS)
                section.Protection |= SectionProtection::Read;
            if (memMapEntry->fPage & VMMDLL_MEMMAP_FLAG_PAGE_W)
                section.Protection |= SectionProtection::Write;
            if (!(memMapEntry->fPage & VMMDLL_MEMMAP_FLAG_PAGE_NX))
                section.Protection |= SectionProtection::Execute;

            if (memMapEntry->wszText[0]) {
                if ((memMapEntry->wszText[0] == 'H' && memMapEntry->wszText[1] == 'E' && memMapEntry->wszText[2] == 'A' &&
                    memMapEntry->wszText[3] == 'P') ||
                    (memMapEntry->wszText[0] == '[' && memMapEntry->wszText[1] == 'H' && memMapEntry->wszText[2] == 'E' &&
                        memMapEntry->wszText[3] == 'A' && memMapEntry->wszText[4] == 'P')) {
                    section.Type = SectionType::Private;

                }
                else {
                    section.Type = SectionType::Image;

                    MultiByteToUnicode(memMapEntry->uszText, section.ModulePath, PATH_MAXIMUM_LENGTH);
                }
            }
            else {
                section.Type = SectionType::Mapped;
            }

            sections.push_back(std::move(section));
        }
        VMMDLL_MemFree(pMemMapEntries);

        DWORD cModuleEntries = 0;
        PVMMDLL_MAP_MODULE pModuleEntries = NULL;

        result = VMMDLL_Map_GetModuleU(_hVmm, dwPID, &pModuleEntries, NULL);

        if (!result) {
            printf( "FAIL: VMMDLL_Map_GetModule\n");

            exit(-1);
        }

        for (i = 0; i < pModuleEntries->cMap; i++) {

            EnumerateRemoteModuleData data = {};
            data.BaseAddress = (RC_Pointer)pModuleEntries->pMap[i].vaBase;
            data.Size = (RC_Size)pModuleEntries->pMap[i].cbImageSize;

            MultiByteToUnicode(pModuleEntries->pMap[i].uszText, data.Path, PATH_MAXIMUM_LENGTH);

            callbackModule(&data);

            // !!!!!!!!!
            // <warning>
            // this code crashes some processes, possibly a bug with vmm.dll
            DWORD cSections = 0;
            PIMAGE_SECTION_HEADER sectionEntry, pSections = NULL;

            result = VMMDLL_ProcessGetSectionsU(_hVmm, dwPID, pModuleEntries->pMap[i].uszText, NULL, 0, &cSections) && cSections &&
                (pSections = (PIMAGE_SECTION_HEADER)malloc(cSections * sizeof(IMAGE_SECTION_HEADER))) &&
                VMMDLL_ProcessGetSectionsU(_hVmm, dwPID, pModuleEntries->pMap[i].uszText, pSections, cSections, &cSections);

            if (result) {
                for (j = 0; j < cSections; j++) {
                    sectionEntry = pSections + j;

                    auto it =
                        std::lower_bound(std::begin(sections), std::end(sections), reinterpret_cast<void*>(pModuleEntries->pMap[i].vaBase),
                            [&sections](const auto& lhs, const void* rhs) { return lhs.BaseAddress < rhs; });

                    auto sectionAddress = (uintptr_t)(pModuleEntries->pMap[i].vaBase + sectionEntry->VirtualAddress);

                    for (auto k = it; k != std::end(sections); ++k) {
                        uintptr_t start = (uintptr_t)k->BaseAddress;
                        uintptr_t end = (uintptr_t)k->BaseAddress + k->Size;

                        if (sectionAddress >= start && sectionAddress < end) {
                            // Copy the name because it is not null padded.
                            char buffer[IMAGE_SIZEOF_SHORT_NAME + 1] = { 0 };
                            std::memcpy(buffer, sectionEntry->Name, IMAGE_SIZEOF_SHORT_NAME);

                            if (std::strcmp(buffer, ".text") == 0 || std::strcmp(buffer, "code") == 0) {
                                k->Category = SectionCategory::CODE;
                            }
                            else if (std::strcmp(buffer, ".data") == 0 || std::strcmp(buffer, "data") == 0 ||
                                std::strcmp(buffer, ".rdata") == 0 || std::strcmp(buffer, ".idata") == 0) {
                                k->Category = SectionCategory::DATA;
                            }
                            MultiByteToUnicode(buffer, k->Name, IMAGE_SIZEOF_SHORT_NAME);
                        }
                    }
                }
            }
            free(pSections);
        }
        VMMDLL_MemFree(pModuleEntries);


        if (callbackSection != nullptr) {
            for (auto&& section : sections) {
                callbackSection(&section);
            }
        }
    
	}
}

extern "C" RC_Export RC_Pointer RC_CallConv OpenRemoteProcess(RC_Pointer id, ProcessAccess desiredAccess) {
	return id;
}

extern "C" RC_Export bool RC_CallConv IsProcessValid(RC_Pointer handle) {
	VMMDLL_PROCESS_INFORMATION info;
	SIZE_T cbInfo = sizeof(VMMDLL_PROCESS_INFORMATION);
	memset(&info, 0, cbInfo);
	info.magic = VMMDLL_PROCESS_INFORMATION_MAGIC;
	info.wVersion = VMMDLL_PROCESS_INFORMATION_VERSION;

    int64_t pid = reinterpret_cast< int64_t >(handle);

    if (pid > 10000000)
        pid -= 10000000;
        
    if (pid == -1)
        return true;

	if (VMMDLL_ProcessGetInformation(_hVmm, static_cast< DWORD >( pid ), &info, &cbInfo)) {
		return true;
	}

	return false;
}

extern "C" RC_Export void RC_CallConv CloseRemoteProcess(RC_Pointer handle)
{
	if (_hVmm)
	{
		VMMDLL_Close(_hVmm);
		_hVmm = NULL;
	}
}

extern "C" RC_Export bool RC_CallConv ReadRemoteMemory(RC_Pointer handle, RC_Pointer address, RC_Pointer buffer, int offset, int size) {
	buffer = reinterpret_cast<RC_Pointer>(reinterpret_cast<uintptr_t>(buffer) + offset);

    int64_t pid = reinterpret_cast< int64_t >(handle);

    if (pid > 10000000)
        pid -= 10000000;

	if (VMMDLL_MemRead(_hVmm, static_cast< DWORD >( pid ) | VMMDLL_PID_PROCESS_WITH_KERNELMEMORY, (ULONG64)address, (PBYTE)buffer, size)) {
		return true;
	}

	return false;
}

extern "C" RC_Export bool RC_CallConv WriteRemoteMemory(RC_Pointer handle, RC_Pointer address, RC_Pointer buffer, int offset, int size)
{
    int64_t pid = reinterpret_cast< int64_t >(handle);

    if (pid < 10000000)
    {
        return false;
    }

    pid -= 10000000;
    
	buffer = reinterpret_cast<RC_Pointer>(reinterpret_cast<uintptr_t>(buffer) + offset);

	if (VMMDLL_MemWrite(_hVmm, static_cast< DWORD >( pid ) | VMMDLL_PID_PROCESS_WITH_KERNELMEMORY, (ULONG64)address, (PBYTE)buffer, size)) {
		return true;
	}

	return false;
}

////////////////////////////////////////
////////////////////////////////////////
// Remote debugging is not supported
////////////////////////////////////////
////////////////////////////////////////

extern "C" RC_Export void RC_CallConv ControlRemoteProcess(RC_Pointer handle, ControlRemoteProcessAction action) {
}

extern "C" RC_Export bool RC_CallConv AttachDebuggerToProcess(RC_Pointer id) {
	return false;
}

extern "C" RC_Export void RC_CallConv DetachDebuggerFromProcess(RC_Pointer id) {
}

extern "C" RC_Export bool RC_CallConv AwaitDebugEvent(DebugEvent * evt, int timeoutInMilliseconds) {
	return false;
}

extern "C" RC_Export void RC_CallConv HandleDebugEvent(DebugEvent * evt) {
}

extern "C" RC_Export bool RC_CallConv SetHardwareBreakpoint(RC_Pointer id, RC_Pointer address, HardwareBreakpointRegister reg, HardwareBreakpointTrigger type, HardwareBreakpointSize size, bool set) {
	return false;
}

extern "C" RC_Export RC_Pointer RC_CallConv InitializeInput() {
    return nullptr;
}

extern "C" RC_Export bool RC_CallConv GetPressedKeys(RC_Pointer handle, int32_t* state[], int* count) {
    return false;
}

extern "C" RC_Export void RC_CallConv ReleaseInput(RC_Pointer handle) {

}

extern "C" RC_Export int32_t RC_CallConv ConnectServer(const char* pIpStr, short port) {
    return 0;
}

extern "C" RC_Export void RC_CallConv DisconnectServer() {

}

extern "C" RC_Export bool RC_CallConv OpenDumpFile(RC_Pointer dumpFilePath){
    return false;
}


#ifdef LINUX
void __attribute__((constructor)) dll_main() {
	
}

void __attribute__((deconstructor)) dll_exit() {
}
#elif defined _WIN32
#endif