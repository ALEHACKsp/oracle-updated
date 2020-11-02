//#include "d3d.h"
//#include <d3d9.h>
//IDirect3D9Ex* p_Object = 0;
//IDirect3DDevice9Ex* p_Device = 0;
//D3DPRESENT_PARAMETERS p_Params;
//
//ID3DXLine* p_Line;
//ID3DXFont* pFontSmall = 0;
//
//int DirectXInit(HWND hWnd)
//{
//	if (FAILED(Direct3DCreate9Ex(D3D_SDK_VERSION, &p_Object)))
//		exit(1);
//
//	ZeroMemory(&p_Params, sizeof(p_Params));
//	p_Params.Windowed = TRUE;
//	p_Params.SwapEffect = D3DSWAPEFFECT_DISCARD;
//	p_Params.hDeviceWindow = hWnd;
//	p_Params.MultiSampleQuality = D3DMULTISAMPLE_NONE;
//	p_Params.BackBufferFormat = D3DFMT_A8R8G8B8;
//	p_Params.EnableAutoDepthStencil = TRUE;
//	p_Params.AutoDepthStencilFormat = D3DFMT_D16;
//
//
//	return 0;
//}
//
//int Render()
//{
//	p_Device->Clear(0, 0, D3DCLEAR_TARGET, 0, 1.0f, 0);
//	p_Device->BeginScene();
//
//
//	p_Device->EndScene();
//	p_Device->PresentEx(0, 0, 0, 0, 0);
//	return 0;
//}
