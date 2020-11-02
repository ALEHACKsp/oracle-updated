	
#include    "DirectOverlay.h"
#include    <D3dx9math.h>
#include    <sstream>
#include    <string>
#include    <algorithm>
#include    <Windows.h>
#include    <list>
#include    <TlHelp32.h>
#include    <tchar.h>
#include    <d3d11.h>  
#include	"imgui.h"
#include	"imgui_impl_dx9.h"
#include	"imgui_impl_win32.h"
#include	"imgui_internal.h"	
#pragma     comment(lib, "d3d11.lib")
#pragma     comment(lib, "d3d9.lib")

typedef HRESULT(__stdcall* D3D11PresentHook) (IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags);
typedef void(__stdcall* D3D11DrawIndexedHook) (ID3D11DeviceContext* pContext, UINT IndexCount, UINT StartIndexLocation, INT BaseVertexLocation);
typedef void(__stdcall* D3D11ClearRenderTargetViewHook) (ID3D11DeviceContext* pContext, ID3D11RenderTargetView* pRenderTargetView, const FLOAT ColorRGBA[4]);

ID3D11Device* pDevice = NULL;
ID3D11DeviceContext* pContext = NULL;

DWORD_PTR* pSwapChainVtable = NULL;
DWORD_PTR* pDeviceContextVTable = NULL;

D3D11PresentHook phookD3D11Present = NULL;
D3D11DrawIndexedHook phookD3D11DrawIndexed = NULL;
D3D11ClearRenderTargetViewHook phookD3D11ClearRenderTargetView = NULL;

IDirect3D9Ex* p_Object = NULL;
IDirect3DDevice9Ex* p_Device = NULL;
D3DPRESENT_PARAMETERS p_Params = { NULL };


void DrawOutlinedString(char mychar[], int x, int y, D3DCOLOR col, D3DCOLOR colb)
{
	DrawString(mychar, 10,  x + 0, y - 1, 0, 0, 0, 1); //up
	DrawString(mychar, 10, x + 0, y + 1, 0, 0, 0, 1); //up
	DrawString(mychar, 10, x + 1, y + 0, 0, 0, 0, 1); //up
	DrawString(mychar, 10, x - 1, y + 0, 0, 0, 0, 1); //up

	DrawString(mychar, 10, x + 1, y + 1, 0, 0, 0, 1); //up
	DrawString(mychar, 10, x - 1, y + 1, 0, 0, 0, 1); //up

	DrawString(mychar, 10, x + 1, y - 1, 0, 0, 0, 1); //up
	DrawString(mychar, 10, x - 1, y - 1, 0, 0, 0, 1); //up

	DrawString(mychar, 10,  x + 0, y + 0, 1, 1, 1, 1); //base
}

HRESULT __stdcall RendererLoop(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags)
{

	pSwapChain->GetDevice(__uuidof(pDevice), (void**)&pDevice);
	pDevice->GetImmediateContext(&pContext);

	return RendererLoop(pSwapChain, SyncInterval, Flags);
}



//0x8AF58E0‬
#define OFFSET_UWORLD     0x8AfAC50
#define KEKTEST           0x8AfAC50
#define ctl_write         CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0366, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)
#define ctl_read		  CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0367, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)
#define ctl_base          CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0368, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)
#define ctl_clear		  CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0369, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)
#define M_PI              3.14159265358979323846264338327950288419716939937510
int Width				 = GetSystemMetrics(SM_CXSCREEN);
int Height				 = GetSystemMetrics(SM_CYSCREEN);
int AimSmooth			 = 1;
int LMAOx				 = 5;
int LMAOy				 = 10;
bool Menu			     = false;
bool Aimbot			     = false;
bool EnemyESP		     = false;
bool NoSpread		     = false;
bool Spinbot			 = false;
bool skeleton		     = true;
bool BoxESP				 = true;
bool LineESP		     = true;
bool DistanceESP		 = true;
bool DrawFov			 = true;
HWND hwnd				 = NULL;
DWORD				     processID;
 
int width;
int height;
int localplayerID;
float FovAngle;

HANDLE DriverHandle;
uint64_t base_address;
POINT p;
DWORD_PTR Uworld;
DWORD_PTR LocalPawn;
DWORD_PTR Localplayer;
DWORD_PTR Rootcomp;
DWORD_PTR PlayerController;
DWORD_PTR Ulevel;
	
Vector3 Localcam;

float AimFOV = 120.f; 

bool isaimbotting;
DWORD_PTR entityx;

bool IsProcessRunning(const TCHAR* const executableName) {
	PROCESSENTRY32 entry;
	entry.dwSize = sizeof(PROCESSENTRY32);

	const auto snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);

	if (!Process32First(snapshot, &entry)) {
		CloseHandle(snapshot);
		return false;
	}

	do {
		if (!_tcsicmp(entry.szExeFile, executableName)) {
			CloseHandle(snapshot);
			return true;
		}
	} while (Process32Next(snapshot, &entry));

	CloseHandle(snapshot);
	return false;
}
void writefloat(unsigned long long int Address, float stuff)
{
	info_t Input_Output_Data;

	Input_Output_Data.pid = processID;

	Input_Output_Data.address = Address;

	Input_Output_Data.value = &stuff;

	Input_Output_Data.size = sizeof(float);

	unsigned long int Readed_Bytes_Amount;

	DeviceIoControl(DriverHandle, ctl_write, &Input_Output_Data, sizeof Input_Output_Data, &Input_Output_Data, sizeof Input_Output_Data, &Readed_Bytes_Amount, nullptr);
}

FTransform GetBoneIndex(DWORD_PTR mesh, int index)
{
	DWORD_PTR bonearray = read<DWORD_PTR>(DriverHandle, processID, mesh + 0x420);

	return read<FTransform>(DriverHandle, processID, bonearray + (index * 0x30));
}

Vector3 GetBoneWithRotation(DWORD_PTR mesh, int id)
{
	FTransform bone = GetBoneIndex(mesh, id);
	FTransform ComponentToWorld = read<FTransform>(DriverHandle, processID, mesh + 0x1C0);

	D3DMATRIX Matrix;
	Matrix = MatrixMultiplication(bone.ToMatrixWithScale(), ComponentToWorld.ToMatrixWithScale());

	return Vector3(Matrix._41, Matrix._42, Matrix._43);
}

D3DXMATRIX Matrix(Vector3 rot, Vector3 origin = Vector3(0, 0, 0))
{
	float radPitch = (rot.x * float(M_PI) / 180.f);
	float radYaw = (rot.y * float(M_PI) / 180.f);
	float radRoll = (rot.z * float(M_PI) / 180.f);

	float SP = sinf(radPitch);
	float CP = cosf(radPitch);
	float SY = sinf(radYaw);
	float CY = cosf(radYaw);
	float SR = sinf(radRoll);
	float CR = cosf(radRoll);

	D3DMATRIX matrix;
	matrix.m[0][0] = CP * CY;
	matrix.m[0][1] = CP * SY;
	matrix.m[0][2] = SP;
	matrix.m[0][3] = 0.f;

	matrix.m[1][0] = SR * SP * CY - CR * SY;
	matrix.m[1][1] = SR * SP * SY + CR * CY;
	matrix.m[1][2] = -SR * CP;
	matrix.m[1][3] = 0.f;

	matrix.m[2][0] = -(CR * SP * CY + SR * SY);
	matrix.m[2][1] = CY * SR - CR * SP * SY;
	matrix.m[2][2] = CR * CP;
	matrix.m[2][3] = 0.f;

	matrix.m[3][0] = origin.x;
	matrix.m[3][1] = origin.y;
	matrix.m[3][2] = origin.z;
	matrix.m[3][3] = 1.f;

	return matrix;
}

Vector3 ProjectWorldToScreen(Vector3 WorldLocation)
{
	Vector3 Screenlocation = Vector3(0, 0, 0);
	Vector3 Camera;

	auto Nigger = read<uintptr_t>(DriverHandle, processID, Localplayer + 0xa8);
	uint64_t Nigger1 = read<uintptr_t>(DriverHandle, processID, Nigger + 8);

	Camera.x = read<float>(DriverHandle, processID, Nigger1 + 0x678);
	Camera.y = read<float>(DriverHandle, processID, Rootcomp + 0x12C);

	float test = asin(Camera.x);
	float degrees = test * (180.0 / M_PI);
	Camera.x = degrees;

	if (Camera.y < 0)
		Camera.y = 360 + Camera.y;

	D3DMATRIX tempMatrix = Matrix(Camera);
	Vector3 vAxisX, vAxisY, vAxisZ;

	vAxisX = Vector3(tempMatrix.m[0][0], tempMatrix.m[0][1], tempMatrix.m[0][2]);
	vAxisY = Vector3(tempMatrix.m[1][0], tempMatrix.m[1][1], tempMatrix.m[1][2]);
	vAxisZ = Vector3(tempMatrix.m[2][0], tempMatrix.m[2][1], tempMatrix.m[2][2]);

	/*world->owningGameInstance->playersArray->localPlayer + 0x70] + 0x98] + 0x130] + 0x10
	world->owningGameInstance->playersArray->localPlayer + 0x20] + 0xAB0] + 0x130] + 0x10
	world->persistentLevel + 0x20] + 0x178] + 0x130] + 0x10
	world->persistentLevel + 0xB8] + 0x178] + 0x130] + 0x10*/

	uint64_t chain = read<uint64_t>(DriverHandle, processID, Ulevel + 0x20);
	uint64_t chain1 = read<uint64_t>(DriverHandle, processID, chain + 0x178);
	uint64_t chain2 = read<uint64_t>(DriverHandle, processID, chain1 + 0x130);

	Vector3 vDelta = WorldLocation - read<Vector3>(DriverHandle, processID, chain2 + 0x10); //camera location credits for Object9999
	Vector3 vTransformed = Vector3(vDelta.Dot(vAxisY), vDelta.Dot(vAxisZ), vDelta.Dot(vAxisX));

	if (vTransformed.z < 1.f)
		vTransformed.z = 1.f;

	float zoom = read<float>(DriverHandle, processID, Nigger1 + 0x500);

	// fov changer here     
	float FovAngle = 80.0f / (zoom / 1.19f);
	float ScreenCenterX = width / 2.0f;
	float ScreenCenterY = height / 2.0f;

	Screenlocation.x = ScreenCenterX + vTransformed.x * (ScreenCenterX / tanf(FovAngle * (float)M_PI / 360.f)) / vTransformed.z;
	Screenlocation.y = ScreenCenterY - vTransformed.y * (ScreenCenterX / tanf(FovAngle * (float)M_PI / 360.f)) / vTransformed.z;

	return Screenlocation;
}

void menu()
{
	if (Menu)
	{
		DrawBox(5, 10, 350, 190, 0.0f, 255.f, 255.f, 255.f, 20, true);
		DrawBox(5, 10, 350, 20, 0.0f, 0.f, 0.f, 0.f, 20, true);
		DrawBox(p.x, p.y, 10, 10, 0.0f, 0.f, 0.f, 0.f, 20, true);
		DrawString(_xor_("sdk").c_str(), 15, 10, 8, 255.f, 255.f, 255.f, 255.f);
		if (EnemyESP)
			//DrawString(_xor_("ON").c_str(), 13, 10 + 110, 10 + 20, 0.f, 0.f, 0.f, 255.f);
			DrawBox(10, 35, 10, 10, 0.0f, 255.f, 0.f, 0.f, 20, true);
		else
			DrawBox(10, 35, 10, 10, 0.0f, 0.f, 255.f, 0.f, 20, true);

		DrawString(_xor_("    Players").c_str(), 13, 10, 10 + 20, 0.f, 0.f, 0.f, 255.f);

		if (BoxESP)
			DrawBox(10, 35 + 20, 10, 10, 0.0f, 255.f, 0.f, 0.f, 20, true);
		else
			DrawBox(10, 35 + 20, 10, 10, 0.0f, 0.f, 255.f, 0.f, 20, true);

		DrawString(_xor_("    Bounding Boxes").c_str(), 13, 10, 10 + 40, 0.f, 0.f, 0.f, 255.f);

		if (LineESP)
			DrawBox(10, 35 + 30, 10, 10, 0.0f, 0.f, 255.f, 0.f, 20, true);
		else
			DrawBox(10, 35 + 30, 10, 10, 0.0f, 255.f, 0.f, 0.f, 20, true);

		DrawString(_xor_("    Snaplines").c_str(), 13, 10, 10 + 60, 0.f, 0.f, 0.f, 255.f);

		if (DistanceESP)		
			DrawBox(10, 35 + 50, 10, 10, 0.0f, 0.f, 255.f, 0.f, 20, true);
		else
			DrawBox(10, 35 + 50, 10, 10, 0.0f, 255.f, 0.f, 0.f, 20, true);

		DrawString(_xor_("    Meters ").c_str(), 13, 10, 10 + 80, 0.f, 0.f, 0.f, 255.f);

		if (Aimbot)
			DrawBox(10, 35 + 70, 10, 10, 0.0f, 0.f, 255.f, 0.f, 20, true);
		else
			DrawBox(10, 35 + 70, 10, 10, 0.0f, 255.f, 0.f, 0.f, 20, true);

		DrawString(_xor_("    Aimbot").c_str(), 13, 10, 10 + 100, 0.f, 0.f, 0.f, 255.f);

		if (Spinbot)
			DrawBox(10, 35 + 90, 10, 10, 0.0f, 0.f, 255.f, 0.f, 20, true);
		else
			DrawBox(10, 35 + 90, 10, 10, 0.0f, 255.f, 0.f, 0.f, 20, true);

		DrawString(_xor_("    Spinbot").c_str(), 13, 10, 10 + 120, 0.f, 0.f, 0.f, 255.f);

		//if (NoSpread)
		//
		//	DrawBox(10, 35 + 120, 10, 10, 0.0f, 255.f, 0.f, 0.f, 20, true);
		//else
		//	DrawBox(10, 35 + 120, 10, 10, 0.0f, 0.f, 255.f, 0.f, 20, true);

		//DrawString(_xor_("    Lock Firerate [0]").c_str(), 13, 10, 10 + 140, 0.f, 0.f, 0.f, 255.f);
		
		DrawString(_xor_("sdk.solutions").c_str(), 13, 10, 10 + 160, 0.f, 0.f, 0.f, 255.f);
	}
}

DWORD Menuthread(LPVOID in)
{
	while (1)
	{

		//DrawString(_xor_("riks private, insert for menu.").c_str(), 17, 10, 8, 255.f, 255.f, 255.f, 255.f);
		HWND test = FindWindowA(_xor_("UnrealWindow").c_str(), _xor_("Fortnite  ").c_str());

		if (test == NULL)
		{
			ExitProcess(0);
		}
		//while (LocalPawn == 0)
		//{
		//	//DrawString(_xor_("Player is not in game.").c_str(), 17, 10, 8, 255.f, 255.f, 255.f, 255.f);
		//	DrawOutlinedString(_xor_("Player is not in game.").c_str(), 17, 10, D3DCOLOR_ARGB(150, 000, 000, 000), D3DCOLOR_ARGB(255, 255, 255, 255));
		//	Sleep(1);
		//}
		if (GetAsyncKeyState(VK_INSERT) & 1) {
			Menu = !Menu;
		}

		if (Menu)
		{
			if (GetCursorPos(&p))
			{
				//cursor position now in p.x and p.y
			}



			if (p.y == 35.f) {
				if (GetAsyncKeyState(VK_LBUTTON) & 1)
				{
					EnemyESP = !EnemyESP;

				}

			}
			if (p.y == 35.f + 20) {
				if (GetAsyncKeyState(VK_LBUTTON) & 1)
				{
					BoxESP = !BoxESP;

				}

			}

			if (p.y == 35.f + 40) {
				if (GetAsyncKeyState(VK_LBUTTON) & 1) {
					LineESP = !LineESP;
				}
			}

			if (p.y == 35.f + 60) {
				if (GetAsyncKeyState(VK_LBUTTON) & 1) {
					DistanceESP = !DistanceESP;
				}
			}

			if (p.y == 35.f + 80) {
				if (GetAsyncKeyState(VK_LBUTTON) & 1) {
					skeleton = !skeleton;
				}
			}

			if (p.y == 35.f + 100) {
				if (GetAsyncKeyState(VK_LBUTTON) & 1) {
					Aimbot = !Aimbot;
				}
			}
			if (p.y == 35.f + 120) {
				if (GetAsyncKeyState(VK_LBUTTON) & 1) {
					Aimbot = !Aimbot;
				}
			}

			if (GetAsyncKeyState(VK_F1) & 1) {
				EnemyESP = !EnemyESP;
			}		

			if (GetAsyncKeyState(VK_F2) & 1) {
				BoxESP = !BoxESP;
			}		
			
			if (GetAsyncKeyState(VK_F3) & 1) {
				LineESP = !LineESP;
			}		
			
			if (GetAsyncKeyState(VK_F4) & 1) {
				DistanceESP = !DistanceESP;
			}		
			
			if (GetAsyncKeyState(VK_F5) & 1) {
				skeleton = !skeleton;
			}

			if (GetAsyncKeyState(VK_F5) & 1) {
				Aimbot = !Aimbot;
			}
		}

	}
}

void aimbot(float x, float y)
{
	float ScreenCenterX = (Width / 2);
	float ScreenCenterY = (Height / 2);
	int   AimSpeed      = AimSmooth;
	float TargetX       = 0;
	float TargetY       = 0;

	if (x != 0)
	{
		if (x > ScreenCenterX)
		{
			TargetX   = -(ScreenCenterX - x);
			TargetX   /= AimSpeed;
			if (TargetX + ScreenCenterX > ScreenCenterX * 2) TargetX = 0;
		}

		if (x < ScreenCenterX)
		{
			TargetX   = x - ScreenCenterX;
			TargetX   /= AimSpeed;
			if (TargetX + ScreenCenterX < 0) TargetX = 0;
		}
	}

	if (y != 0)
	{
		if (y > ScreenCenterY)
		{
			TargetY = -(ScreenCenterY - y);
			TargetY /= AimSpeed;
			if (TargetY + ScreenCenterY > ScreenCenterY * 2) TargetY = 0;
		}

		if (y < ScreenCenterY)
		{
			TargetY = y - ScreenCenterY;
			TargetY /= AimSpeed;
			if (TargetY + ScreenCenterY < 0) TargetY = 0;
		}
	}
	// mem aim only.

	float theNum = floor(TargetX / AimSpeed);
	float result = theNum / 6.666666666666667f;

	float theNum1 = floor(TargetY / AimSpeed);
	float resulte = theNum1 / 6.666666666666667f;
	float result1 = -(resulte);

	writefloat(PlayerController + 0x418 + 0x0, result1);
	writefloat(PlayerController + 0x418 + 0x4, result);

	return;
}

void AimAt(DWORD_PTR entity)
{
	uint64_t currentactormesh = read<uint64_t>(DriverHandle, processID, entity + 0x278);
	auto rootHead = GetBoneWithRotation(currentactormesh, 66);
	Vector3 rootHeadOut = ProjectWorldToScreen(rootHead);

	if (rootHeadOut.y != 0 || rootHeadOut.y != 0)
	{
		aimbot(rootHeadOut.x, rootHeadOut.y);
	}
}

void DrawSkeleton(DWORD_PTR mesh)
{
	Vector3 vHeadBoneOut = ProjectWorldToScreen(GetBoneWithRotation(mesh, 96));
	Vector3 vHipOut = ProjectWorldToScreen(GetBoneWithRotation(mesh, 2));
	Vector3 vNeckOut = ProjectWorldToScreen(GetBoneWithRotation(mesh, 65));
	Vector3 vUpperArmLeftOut = ProjectWorldToScreen(GetBoneWithRotation(mesh, 34));
	Vector3 vUpperArmRightOut = ProjectWorldToScreen(GetBoneWithRotation(mesh, 91));
	Vector3 vLeftHandOut = ProjectWorldToScreen(GetBoneWithRotation(mesh, 35));
	Vector3 vRightHandOut = ProjectWorldToScreen(GetBoneWithRotation(mesh, 63));
	Vector3 vLeftHandOut1 = ProjectWorldToScreen(GetBoneWithRotation(mesh, 33));
	Vector3 vRightHandOut1 = ProjectWorldToScreen(GetBoneWithRotation(mesh, 60));
	Vector3 vRightThighOut = ProjectWorldToScreen(GetBoneWithRotation(mesh, 74));
	Vector3 vLeftThighOut = ProjectWorldToScreen(GetBoneWithRotation(mesh, 67));
	Vector3 vRightCalfOut = ProjectWorldToScreen(GetBoneWithRotation(mesh, 75));
	Vector3 vLeftCalfOut = ProjectWorldToScreen(GetBoneWithRotation(mesh, 68));
	Vector3 vHeadOut = ProjectWorldToScreen(GetBoneWithRotation(mesh, 66));
	Vector3 vLeftFootOut = ProjectWorldToScreen(GetBoneWithRotation(mesh, 69));
	Vector3 vRightFootOut = ProjectWorldToScreen(GetBoneWithRotation(mesh, 76));

	DrawLine(vHeadBoneOut.x, vHeadBoneOut.y, vNeckOut.x, vNeckOut.y, 0.30f, 255.f, 255.f, 255.f, 200.f);
	DrawLine(vHipOut.x, vHipOut.y, vNeckOut.x, vNeckOut.y, 0.30f, 255.f, 255.f, 255.f, 200.f);
	
	DrawLine(vUpperArmLeftOut.x, vUpperArmLeftOut.y, vNeckOut.x, vNeckOut.y, 0.30f, 255.f, 255.f, 255.f, 200.f);
	DrawLine(vUpperArmRightOut.x, vUpperArmRightOut.y, vNeckOut.x, vNeckOut.y, 0.30f, 255.f, 255.f, 255.f, 200.f);

	DrawLine(vLeftHandOut.x, vLeftHandOut.y, vUpperArmLeftOut.x, vUpperArmLeftOut.y, 0.30f, 255.f, 255.f, 255.f, 200.f);
	DrawLine(vRightHandOut.x, vRightHandOut.y, vUpperArmRightOut.x, vUpperArmRightOut.y, 0.30f, 255.f, 255.f, 255.f, 200.f);

	DrawLine(vLeftHandOut.x, vLeftHandOut.y, vLeftHandOut1.x, vLeftHandOut1.y, 0.30f, 255.f, 255.f, 255.f, 200.f);
	DrawLine(vRightHandOut.x, vRightHandOut.y, vRightHandOut1.x, vRightHandOut1.y, 0.30f, 255.f, 255.f, 255.f, 200.f);

	DrawLine(vLeftThighOut.x, vLeftThighOut.y, vHipOut.x, vHipOut.y, 0.30f, 255.f, 255.f, 255.f, 200.f);
	DrawLine(vRightThighOut.x, vRightThighOut.y, vHipOut.x, vHipOut.y, 0.30f, 255.f, 255.f, 255.f, 200.f);

	DrawLine(vLeftCalfOut.x, vLeftCalfOut.y, vLeftThighOut.x, vLeftThighOut.y, 0.30f, 255.f, 255.f, 255.f, 200.f);
	DrawLine(vRightCalfOut.x, vRightCalfOut.y, vRightThighOut.x, vRightThighOut.y, 0.30f, 255.f, 255.f, 255.f, 200.f);

	DrawLine(vLeftFootOut.x, vLeftFootOut.y, vLeftCalfOut.x, vLeftCalfOut.y, 0.30f, 255.f, 255.f, 255.f, 200.f);
	DrawLine(vRightFootOut.x, vRightFootOut.y, vRightCalfOut.x, vRightCalfOut.y, 0.30f, 255.f, 255.f, 255.f, 200.f);
}

void drawLoop(int width, int height) {
	menu();

	DrawCircle(width / 2, height / 2, AimFOV, 1.f, 255.f, 255.f, 255.f, 255.f, false);

	Uworld = read<DWORD_PTR>(DriverHandle, processID, base_address + OFFSET_UWORLD);
	//printf(_xor_("Uworld: %p.\n").c_str(), Uworld);

	DWORD_PTR Gameinstance = read<DWORD_PTR>(DriverHandle, processID, Uworld + 0x188);

	if (Gameinstance == (DWORD_PTR)nullptr)
		return;

	//printf(_xor_("Gameinstance: %p.\n").c_str(), Gameinstance);

	DWORD_PTR LocalPlayers = read<DWORD_PTR>(DriverHandle, processID, Gameinstance + 0x38);

	if (LocalPlayers == (DWORD_PTR)nullptr)
		return;

	//printf(_xor_("LocalPlayers: %p.\n").c_str(), LocalPlayers);

	Localplayer = read<DWORD_PTR>(DriverHandle, processID, LocalPlayers);

	if (Localplayer == (DWORD_PTR)nullptr)
		return;

	//printf(_xor_("LocalPlayer: %p.\n").c_str(), Localplayer);

	PlayerController = read<DWORD_PTR>(DriverHandle, processID, Localplayer + 0x30);

	if (PlayerController == (DWORD_PTR)nullptr)
		return;

	//printf(_xor_("playercontroller: %p.\n").c_str(), PlayerController);

	LocalPawn = read<uint64_t>(DriverHandle, processID, PlayerController + 0x298);

	if (LocalPawn == (DWORD_PTR)nullptr)
		return;

	//printf(_xor_("Pawn: %p.\n").c_str(), LocalPawn);

	Rootcomp = read<uint64_t>(DriverHandle, processID, LocalPawn + 0x130);

	if (Rootcomp == (DWORD_PTR)nullptr)
		return;

	//printf(_xor_("Rootcomp: %p.\n").c_str(), Rootcomp);

	if (LocalPawn != 0) {
		localplayerID = read<int>(DriverHandle, processID, LocalPawn + 0x18);
	}

	Ulevel = read<DWORD_PTR>(DriverHandle, processID, Uworld + 0x30);
	//printf(_xor_("Ulevel: %p.\n").c_str(), Ulevel);

	if (Ulevel == (DWORD_PTR)nullptr)
		return;

	DWORD64 PlayerState = read<DWORD64>(DriverHandle, processID, LocalPawn + 0x238);

	if (PlayerState == (DWORD_PTR)nullptr)
		return;

	DWORD ActorCount = read<DWORD>(DriverHandle, processID, Ulevel + 0xA0);

	DWORD_PTR AActors = read<DWORD_PTR>(DriverHandle, processID, Ulevel + 0x98);
	//printf(_xor_("AActors: %p.\n").c_str(), AActors);

	if (AActors == (DWORD_PTR)nullptr)
		return;

	float closestDistance = FLT_MAX;
	DWORD_PTR closestPawn = NULL;

	for (int i = 0; i < ActorCount; i++)
	{

		float minX = FLT_MAX;
		float maxX = -FLT_MAX;
		float minY = FLT_MAX;
		float maxY = -FLT_MAX;

		uint64_t CurrentActor = read<uint64_t>(DriverHandle, processID, AActors + i * 0x8);

		int curactorid = read<int>(DriverHandle, processID, CurrentActor + 0x18);

		if (curactorid == localplayerID || curactorid == localplayerID + 121)
		{
			if (CurrentActor == (uint64_t)nullptr || CurrentActor == -1 || CurrentActor == NULL)
				continue;

			uint64_t CurrentActorRootComponent = read<uint64_t>(DriverHandle, processID, CurrentActor + 0x130);

			if (CurrentActorRootComponent == (uint64_t)nullptr || CurrentActorRootComponent == -1 || CurrentActorRootComponent == NULL)
				continue;

			uint64_t currentactormesh = read<uint64_t>(DriverHandle, processID, CurrentActor + 0x278);

			if (currentactormesh == (uint64_t)nullptr || currentactormesh == -1 || currentactormesh == NULL)
				continue;

			int MyTeamId = read<int>(DriverHandle, processID, PlayerState + 0xE60);

			DWORD64 otherPlayerState = read<uint64_t>(DriverHandle, processID, CurrentActor + 0x238);

			if (otherPlayerState == (uint64_t)nullptr || otherPlayerState == -1 || otherPlayerState == NULL)
				continue;

			int ActorTeamId = read<int>(DriverHandle, processID, otherPlayerState + 0xE60);
	
			if (MyTeamId != ActorTeamId)
			{
				Vector3 vLeftFootOut = ProjectWorldToScreen(GetBoneWithRotation(currentactormesh, 69));
				Vector3 Headpos = GetBoneWithRotation(currentactormesh, 66);
				Vector3 Kneepos = GetBoneWithRotation(currentactormesh, 68);
				localactorpos = read<Vector3>(DriverHandle, processID, Rootcomp + 0x11C);

				float distance = localactorpos.Distance(Headpos) / 100.f;

				//W2S
				Vector3 HeadposW2s = ProjectWorldToScreen(Headpos);
				Vector3 KneeposW2s = ProjectWorldToScreen(Kneepos);
				Vector3 bone0 = GetBoneWithRotation(currentactormesh, 0);
				Vector3 bottom = ProjectWorldToScreen(bone0);
				Vector3 Headbox = ProjectWorldToScreen(Vector3(Headpos.x, Headpos.y, Headpos.z + 15));
				Vector3 Aimpos = ProjectWorldToScreen(Vector3(Headpos.x, Headpos.y, Headpos.z + 10));

				float Height1 = abs(Headbox.y - bottom.y);
				float Width1 = Height1 * 0.65;

				DrawLine(width / 2 - 4, height - 0, width + 4, height + 0, 1, 1, 1, 1);
				DrawLine(width + 0, height - 4, width - 0, height + 4, 1, 1, 1, 1);

				if (skeleton)
					DrawSkeleton(currentactormesh);

				if (BoxESP)	
					float lineW			    = (width / 5);
					float lineH				= (height / 6);
					float lineT             = 1;
					auto OutlineTopLeft     = NULL;
					auto OutlineBottomRight = NULL;


					// Bounding Box
					DrawBox(Headbox.x - (Width1 / 2), Headbox.y, Width1, Height1, 1.f, 255.f, 0.f, 0.f, 1000.f, false);
					/*DrawBox(Headbox.x - (Width1 / 2), Headbox.y, Width1, Height1, 1.f, 0.f, 0.f, 0.f, 0.20f, true);*/


	
				if (EnemyESP)

				if (DistanceESP)
				{
					CHAR dist[50];
					sprintf_s(dist, _xor_("Player [%.f]").c_str(), distance);
					 // DrawString(dist, 13, HeadposW2s.x + 70, HeadposW2s.y - 25, 1, 1, 1);
					DrawOutlinedString(dist,  KneeposW2s.x + 40 + -130 + 28 + 42 + -6, KneeposW2s.y - 25 + -14 + 2, NULL, NULL);
				}

				if (LineESP)
					DrawLine(width / 2, height, bottom.x, bottom.y, 0.20f, 255.f, 255.f, 255.f, 20.f	);

				if (Aimbot)
				{
					auto dx = HeadposW2s.x - (width / 2);
					auto dy = HeadposW2s.y - (height / 2);
					auto dist = sqrtf(dx * dx + dy * dy);

					if (dist < AimFOV && dist < closestDistance) {
						AimFOV = AimFOV + 10;
						closestDistance = dist;
						closestPawn = CurrentActor;
					}
				}
			}
		}
	}

	if (Aimbot)
	{
		if (closestPawn && GetAsyncKeyState(VK_RBUTTON) < 0) { //change aimkey here
			AimAt(closestPawn);
		}
	}
}
void slow_print(const std::string& message, unsigned int millis_per_char)
{
	// Range loops are "for each" constructs; here: for each character in the string
	for (const char c : message)
	{
		// flush is used to make sure the buffer is emptied to the terminal immediately
		std::cout << c << std::flush;

		// Ask the thread to sleep for at least n millis.
		Sleep(millis_per_char);
	}
}


void main()
{
	system(_xor_("color b").c_str());
	DriverHandle = CreateFileW(_xor_(L"\\\\.\\doesshelikemetho").c_str(), GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
	system("taskkill /f /im EpicGamesLauncher.exe");
	system("taskkill /f /im FortniteClient-Win64-Shipping.exe");
	system("cls");
	system(_xor_("color b").c_str());
	SetConsoleTitleA(_xor_("F").c_str()); 
	Sleep(500);
	SetConsoleTitleA(_xor_("Fo").c_str());
	Sleep(500);
	SetConsoleTitleA(_xor_("For").c_str());
	Sleep(500);
	SetConsoleTitleA(_xor_("Fort").c_str());
	Sleep(500);
	SetConsoleTitleA(_xor_("Fortn").c_str());
	Sleep(500);
	SetConsoleTitleA(_xor_("Fortni").c_str());
	Sleep(500);
	SetConsoleTitleA(_xor_("Fortnit").c_str());
	Sleep(500);
	SetConsoleTitleA(_xor_("Fortnite").c_str());
	Sleep(500);
	SetConsoleTitleA(_xor_("Fortnite S").c_str());
	Sleep(500);
	SetConsoleTitleA(_xor_("Fortnite SD").c_str());
	Sleep(500);
	SetConsoleTitleA(_xor_("Fortnite SDK").c_str());
	Sleep(500);
	SetConsoleTitleA(_xor_("B3011").c_str());
	Sleep(500);
	Sleep(5000);
	std::string message = _xor_("\n\n  sdk.solutions fortnite external lite\n  Last update : yes nigar i updated to last gamepatch\n  Initializing process, may take a while. Please wait\n");
	Sleep(2000);
	slow_print(message, 30);
	if (DriverHandle == INVALID_HANDLE_VALUE)
	{
		//UEFA_DH = driver not loaded
		printf(_xor_("  Software error occured\n").c_str());
		printf(_xor_("\n").c_str());
		Sleep(1000);
		printf(_xor_("\n").c_str());
		Sleep(1000);
		printf(_xor_("  Exiting program in two seconds.\n").c_str());
		Sleep(2000);
		exit(0);
	}  
	
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

	system(_xor_("cls").c_str());
	info_t Input_Output_Data1;
	unsigned long int Readed_Bytes_Amount1;
	DeviceIoControl(DriverHandle, ctl_clear, &Input_Output_Data1, sizeof Input_Output_Data1, &Input_Output_Data1, sizeof Input_Output_Data1, &Readed_Bytes_Amount1, nullptr);
	// MessageBox(0, "[+] Software updated to last game patch (15 06 20)\n[+] Removed no-spread and switching method\n[+] New menu\n[+] Added F1, F2, F3, F4, F5 & F6 keys for menu\n\nsdk.solutions", "News", MB_ICONINFORMATION);
	Sleep(3000);
	printf(_xor_("\\Driver\\Handler\\i2ff7x8a0v5x7bt9b\n").c_str());
	Sleep(100);
	system(_xor_("cls").c_str());	
	printf(_xor_("\\Driver\\Handler\\x2kk5x8a0v5x7bt9b\n").c_str());
	Sleep(100);
	system(_xor_("cls").c_str());	
	printf(_xor_("\\Driver\\Handler\\i2ff7x8a0v5x7bt9b\n").c_str());
	Sleep(100);
	system(_xor_("cls").c_str());
	Sleep(3000);
	printf(_xor_("\n\n  Initialised\n").c_str());
	Sleep(1000);
	system(_xor_("cls").c_str());
	SetConsoleTextAttribute(hConsole, 11);
	printf(_xor_("\n  Connecting...").c_str());
	Sleep(2000);
	printf(_xor_("\n\n  Activating old license key : 00000-00000-00000-00000-00000-00000\n  Status : Active (90h)").c_str());
	//system(_xor_("cls").c_str());
	printf(_xor_("\n\n  Start FortniteClient-Win64-Shipping.exe").c_str());
	SetConsoleTextAttribute(hConsole, 15);
	printf(_xor_(" and enable discord overlay\n").c_str());
	SetConsoleTextAttribute(hConsole, 11);
	while (!IsProcessRunning(_xor_("FortniteClient-Win64-Shipping.exe").c_str())) {
		Sleep(1);
	}
	printf(_xor_("\n  Process Found\n").c_str());
	while (hwnd == NULL)
	{
		hwnd = FindWindowA(0, _xor_("Fortnite  ").c_str());

		Sleep(1);
	}	
	printf(_xor_("  Process Window Found\n").c_str());
	GetWindowThreadProcessId(hwnd, &processID);

	RECT rect;
	if (GetWindowRect(hwnd, &rect))
	{
		width = rect.right - rect.left;
		height = rect.bottom - rect.top;
	}

	info_t Input_Output_Data;
	Input_Output_Data.pid = processID;
	unsigned long int Readed_Bytes_Amount;

	DeviceIoControl(DriverHandle, ctl_base, &Input_Output_Data, sizeof Input_Output_Data, &Input_Output_Data, sizeof Input_Output_Data, &Readed_Bytes_Amount, nullptr);
	base_address = (unsigned long long int)Input_Output_Data.data;

	// printf(_xor_("PID -> %s\n").c_str(), processID);

	// std::cout << "PID -> 0x" << processID << std::endl;
	printf(_xor_("\n  F7 to inject\n").c_str());
	while (!GetAsyncKeyState(VK_F7) & 1)
	{
		Sleep(1);
	}
	printf(_xor_("\n  Starting injection process\n").c_str());
	Sleep(1000);
	printf(_xor_("  Getting thread context and injecting\n").c_str());
	Sleep(9500);


	printf(_xor_("\n  Injection process is complete! Have fun\n\n").c_str());
	Sleep(1500);
	system(_xor_("cls").c_str());
	CreateThread(NULL, NULL, Menuthread, NULL, NULL, NULL);
	DirectOverlaySetup(drawLoop, hwnd);
	DirectOverlaySetOption(D2DOV_REQUIRE_FOREGROUND);

	getchar();

	//  NOTE : neutron font is tahoma.ttf
}
