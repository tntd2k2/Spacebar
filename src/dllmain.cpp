#ifdef _MSC_VER
#include <intrin.h>
#endif
#include <math.h>
#include <windows.h>
#include <thread>
#include <d3d11.h>
#include <dxgi.h>
#include <iostream>
#include <cstdint>
#include <psapi.h>
#include <string>
#include_next <intrin.h>

#include "Decrypt\Decrypt.h"
#include "Hooks\UltimateHooks.h"
#include "Console\Console.h"
#include "..\libraries\kiero\kiero.h"
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"

#define BaseAddress (DWORD64)GetModuleHandle(0)
#define StubBaseAddress (DWORD64)GetModuleHandle(L"stub.dll")
#define HiddenBaseAddress (DWORD64)FindHiddenModule()
#define DEFINE_RVA(Address) (BaseAddress + (DWORD64)Address)
#define DEFINE_STUB_RVA(Address) (StubBaseAddress + (DWORD64)Address)
#define DEFINE_HIDDEN_RVA(Address) (HiddenBaseAddress + (DWORD64)Address)

#define STR_MERGE_IMPL(x, y)                x##y
#define STR_MERGE(x,y)                        STR_MERGE_IMPL(x,y)
#define MAKE_PAD(size)                        BYTE STR_MERGE(pad_, __COUNTER__) [ size ]
#define DEFINE_MEMBER_0(x)                    x;
#define DEFINE_MEMBER_N(x,offset)            struct { MAKE_PAD((DWORD)offset); x; };

typedef HRESULT(__stdcall* Present) (IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags);
static Present oPresent = NULL;
typedef LRESULT(CALLBACK* WNDPROC)(HWND, UINT, WPARAM, LPARAM);
typedef uintptr_t PTR;
extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

HMODULE g_module;
uintptr_t initThreadHandle;
HWND window = NULL;
WNDPROC oWndProc;
ID3D11Device* pDevice = NULL;
ID3D11DeviceContext* pContext = NULL;
ID3D11RenderTargetView* mainRenderTargetView;
Console console;

DWORD64 FindHiddenModule() {
    MEMORY_BASIC_INFORMATION64 mbi = { 0 };
    DWORD64 start = 0;
    DWORD64 result = 0;

    while (VirtualQuery(reinterpret_cast<LPVOID>(start), reinterpret_cast<PMEMORY_BASIC_INFORMATION>(&mbi), sizeof(MEMORY_BASIC_INFORMATION64)) != 0) {
        if (mbi.Protect & PAGE_READWRITE && mbi.State == MEM_COMMIT && mbi.Type == 0x40000) {
            result = reinterpret_cast<DWORD64>(mbi.AllocationBase);
            if (*reinterpret_cast<WORD*>(result) == 0x5A4D) {
                const auto dosHeader = reinterpret_cast<PIMAGE_DOS_HEADER>(result);
                const auto ntPtr = reinterpret_cast<PIMAGE_NT_HEADERS64>(result + dosHeader->e_lfanew);
                if (ntPtr->FileHeader.NumberOfSections == 10)
                    break;
            }
        }
        start += mbi.RegionSize;
    }
    return result;
}

static const ImWchar tahomaRanges[] = {
	0x0020, 0x00FF, // Basic Latin + Latin Supplement
	0x0100, 0x024F, // Latin Extended-A + Latin Extended-B
	0x0250, 0x02FF, // IPA Extensions + Spacing Modifier Letters
	0x0300, 0x03FF, // Combining Diacritical Marks + Greek/Coptic
	0x0400, 0x052F, // Cyrillic + Cyrillic Supplement
	0x0530, 0x06FF, // Armenian + Hebrew + Arabic
	0x0E00, 0x0E7F, // Thai
	0x1E00, 0x1FFF, // Latin Extended Additional + Greek Extended
	0x2000, 0x20CF, // General Punctuation + Superscripts and Subscripts + Currency Symbols
	0x2100, 0x218F, // Letterlike Symbols + Number Forms
	0,
};


void InitImGui()
{
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	ImFontConfig cfg;
	cfg.SizePixels = 15.0f;
	io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
	io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\tahoma.ttf", cfg.SizePixels, &cfg, tahomaRanges);
	ImGui_ImplWin32_Init(window);
	ImGui_ImplDX11_Init(pDevice, pContext);
}

LRESULT __stdcall WndProc(const HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {

	if (true && ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam))
		return true;

	return CallWindowProc(oWndProc, hWnd, uMsg, wParam, lParam);
}

bool init = false;
HRESULT __stdcall hkPresent(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags)
{
	if (!init)
	{
		if (SUCCEEDED(pSwapChain->GetDevice(__uuidof(ID3D11Device), (void**)&pDevice)))
		{
			pDevice->GetImmediateContext(&pContext);
			DXGI_SWAP_CHAIN_DESC sd;
			pSwapChain->GetDesc(&sd);
			window = sd.OutputWindow;
			ID3D11Texture2D* pBackBuffer;
			pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
			pDevice->CreateRenderTargetView(pBackBuffer, NULL, &mainRenderTargetView);
			pBackBuffer->Release();
			oWndProc = (WNDPROC)SetWindowLongPtr(window, GWLP_WNDPROC, (LONG_PTR)WndProc);
			InitImGui();
			init = true;
		}

		else
			return oPresent(pSwapChain, SyncInterval, Flags);
	}

	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
	ImGui::SetNextWindowSizeConstraints(ImVec2(500, 500), ImVec2(800, 800));
	console.Render();

	ImGui::End();

	ImGui::EndFrame();

	ImGui::Render();

	pContext->OMSetRenderTargets(1, &mainRenderTargetView, NULL);
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
	return oPresent(pSwapChain, SyncInterval, Flags);
}

void* VirtualAllocateRegion(void* TargetAddress) {
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    const size_t pageSize = sysInfo.dwPageSize;
    DWORD64 minAddress = reinterpret_cast<DWORD64>(sysInfo.lpMinimumApplicationAddress);
    DWORD64 maxAddress = reinterpret_cast<DWORD64>(sysInfo.lpMaximumApplicationAddress);
    DWORD64 startPage = uint64_t(TargetAddress) & ~(pageSize - 1);
    void* retAddress;
    for (DWORD64 pageOffset = 0; pageOffset < 0x7FFFFF; ++pageOffset) {
        DWORD64 byteOffset = pageOffset * pageSize;
        DWORD64 highAddress = startPage + byteOffset;
        DWORD64 lowAddress = (startPage > byteOffset) ? startPage - byteOffset : 0;
        if (highAddress < maxAddress)
            retAddress = VirtualAlloc(reinterpret_cast<void*>(highAddress), pageSize, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);

        if (lowAddress > minAddress)
            retAddress = VirtualAlloc(reinterpret_cast<void*>(lowAddress), pageSize, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);

        if (retAddress != nullptr)
            return retAddress;
    }
    return nullptr;
}

class LeagueHash {
public:
    union {
        DEFINE_MEMBER_N(uint8_t temp, 0x69);
        DEFINE_MEMBER_N(uint64_t xor_key, 0xB0);
    };

    uint64_t xored_hash(void) {
        return *reinterpret_cast<std::uint64_t*>(uint64_t(this) + temp * 8 + 0x70);
    };

    void xored_hash(uint64_t hashed_value) {
        *reinterpret_cast<std::uint64_t*>(uint64_t(this) + temp * 8 + 0x70) = hashed_value;
    };

};

uint64_t fnv1a64(void* data, size_t size) {
    uint64_t FNV_PRIME = 0x100000001B3;
    uint64_t FNV_OFFSET_BASIS = 0xCBF29CE484222325;

    uint8_t* bytes = static_cast<uint8_t*>(data);
    uint64_t hash = FNV_OFFSET_BASIS;

    for (size_t i = 0; i < size; ++i) {
        hash ^= static_cast<uint64_t>(bytes[i]);
        hash *= FNV_PRIME;
    }

    return hash;
}

void WriteJump(void* hookAddress, void* hookDestination) {
    uint8_t jmpInstruction[] = {
         0xFF, 0x25, 0x00, 0x00, 0x00, 0x00,
         0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    };
    uint64_t addrToJumpTo64 = reinterpret_cast<uint64_t>(hookDestination);
    memcpy(&jmpInstruction[6], &addrToJumpTo64, sizeof(addrToJumpTo64));
    memcpy(hookAddress, &jmpInstruction, sizeof(jmpInstruction));
}

bool HookLeague(void* hookAddress, void* hookDestination, size_t hookSize) {
    uint64_t hkOffset = reinterpret_cast<uint64_t>(hookAddress) - BaseAddress;
    int pageIndex = floor(hkOffset / 0x1000);
    uint8_t arrBytes[0x1000];
    uint64_t currentPage = BaseAddress + pageIndex * 0x1000;
    uint64_t currentHiddenPage = HiddenBaseAddress + pageIndex * 0x1000;
    for (int i = 0; i < 0x1000; i++)
        arrBytes[i] = *reinterpret_cast<uint8_t*>(currentPage + i);
    // 13.22 = 0x49E9C0
    // 13.23 = 0x2D1410
    LeagueHash* pageHashedArray = (LeagueHash*)(DEFINE_STUB_RVA(0x2D1410) + pageIndex * 0x140);
    void* NewHkDestination = VirtualAllocateRegion(hookAddress);
    WriteJump(NewHkDestination, hookDestination);

    uint8_t jmpInstruction[5] = { 0xE9, 0x0, 0x0, 0x0, 0x0 };
    if (hookSize < sizeof(jmpInstruction))
        return false;
    DWORD curProtection;

    VirtualProtect(hookAddress, hookSize, PAGE_EXECUTE_READWRITE, &curProtection);
    uint64_t relativeAddress = reinterpret_cast<uint64_t>(NewHkDestination) - (reinterpret_cast<uint64_t>(hookAddress) + sizeof(jmpInstruction));
    memcpy(jmpInstruction + 1, &relativeAddress, 4);
    uint64_t patchOffset = reinterpret_cast<uint64_t>(hookAddress) - currentPage;
    memset(&arrBytes[patchOffset], 0x90, hookSize);
    memcpy(&arrBytes[patchOffset], jmpInstruction, sizeof(jmpInstruction));
    memset(reinterpret_cast<void*>(currentHiddenPage + patchOffset), 0x90, hookSize);
    memcpy(reinterpret_cast<void*>(currentHiddenPage + patchOffset), &jmpInstruction, sizeof(jmpInstruction));
    pageHashedArray->xored_hash(fnv1a64(arrBytes, sizeof(arrBytes)) ^ pageHashedArray->xor_key);
    return true;
}

class LolString {
    char content[0x10];
    uint64_t length = 0;

public:
    operator const char* (void) {
        if (uint64_t(this) <= 0x1000)
            return "";

        return length >= 0x10 ? *reinterpret_cast<char**>(content) : content;
    }
};

class SpellInfo
{
public:
    DEFINE_MEMBER_N(LolString SpellName, 0x28);
};

class SpellCastInfo
{
public:
    SpellInfo* SpellInfo;
};

class GameObject
{
public:
    DEFINE_MEMBER_N(LolString Name, 0x60);
};

typedef void(__fastcall* fnOnProcessSpell)(void* SpellBook, SpellCastInfo* castinfo);
fnOnProcessSpell pOnProcessSpell;

typedef int(__fastcall* fnOnCreateObject)(GameObject* NewObject, unsigned int netId);
fnOnCreateObject pOnCreateObject;

void __fastcall OnCreateObject(GameObject* NewObject, unsigned int netId) {
    const char* Name = NewObject->Name;
    std::string strName(Name);
    console.Print("%s", strName.c_str());
    
}

void __fastcall OnStartCastSpell(void* spell_book, SpellCastInfo* spellCastInfo) {
    const char* SpellName = spellCastInfo->SpellInfo->SpellName;
    console.Print("%s", SpellName);
}

uintptr_t OnStartCastSpellJmpBackAddr;
__declspec(naked) void hkOnStartCastSpell() {
    __asm {
        pushf
        push rax
        push rcx
        push rdx
        push rbx
        push rsp
        push rbp
        push rsi
        push rdi
        push r8
        push r9
        push r10
        push r11
        push r12
        push r13
        push r14
        push r15

        call OnStartCastSpell

        pop r15
        pop r14
        pop r13
        pop r12
        pop r11
        pop r10
        pop r9
        pop r8
        pop rdi
        pop rsi
        pop rbp
        pop rsp
        pop rbx
        pop rdx
        pop rcx
        pop rax
        popf

        push rbx
        sub rsp, 0x30
        jmp OnStartCastSpellJmpBackAddr
    }
}



bool hook_stub_check() {
    uint64_t OnStartCastSpellRVA = DEFINE_RVA(0x720FE0);
    OnStartCastSpellJmpBackAddr = OnStartCastSpellRVA + 6;
    if (!HookLeague(reinterpret_cast<void*>(OnStartCastSpellRVA), reinterpret_cast<void*>(hkOnStartCastSpell), 6))
        return false;

    return true;

}

DWORD WINAPI MainThread(LPVOID param) {
	UltimateHooks::RestoreZwProtectVirtualMemory();
	UltimateHooks::RestoreZwQueryVirtualMemory();
	//LeagueDecrypt::Decrypt();

	bool init_hook = false;
	do
	{
		if (kiero::init(kiero::RenderType::D3D11) == kiero::Status::Success)
		{
			kiero::bind(8, (void**)&oPresent, hkPresent);
			init_hook = true;
		}
	} while (!init_hook);


	if (GetSystemDEPPolicy() != DEP_SYSTEM_POLICY_TYPE::DEPPolicyAlwaysOff) {
		SetProcessDEPPolicy(PROCESS_DEP_ENABLE);
		console.Show();
        if (!hook_stub_check()) {
            MessageBoxA(NULL, "Failed to hook!", "Error", MB_OK);
            return FALSE;
        }
	}
    return 1;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD dwReason, LPVOID lpvReserved) {
    DisableThreadLibraryCalls(hModule);

    switch (dwReason) {
    case DLL_PROCESS_ATTACH:
        initThreadHandle = _beginthreadex(nullptr, 0, (_beginthreadex_proc_type)MainThread, hModule, 0, nullptr);
        break;
    case DLL_THREAD_ATTACH:
        break;
    case DLL_THREAD_DETACH:
        break;
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}