#pragma once

#include <windows.h>
#include <tchar.h>
#include <sstream>
#include <wrl.h>

#include "misc.h"
#include "high_resolution_timer.h"
#include "framebuffer.h"
#include "fullscreen_quad.h"
#include "shader.h"
#include "sprite.h"
#include "sprite_batch.h"
#include "geometric_primitive.h"
#include "static_mesh.h"
#include "skinned_mesh.h"

#include <d3d11.h>

#ifdef USE_IMGUI
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"
#include "imgui/imgui_impl_dx11.h"
#include "imgui/imgui_impl_win32.h"
extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
extern ImWchar glyphRangesJapanese[];
#endif

#include <wrl.h>
#include "audio.h"

CONST LONG SCREEN_WIDTH{ 1280 };
CONST LONG SCREEN_HEIGHT{ 720 };
CONST BOOL FULLSCREEN{ FALSE };
CONST LPCWSTR APPLICATION_NAME{ L"X3DGP" };

class framework
{
private:
	// ImGui用変数

	DirectX::XMMATRIX world;

	float eyeX = 0.0f, eyeY = 0.0f, eyeZ = -10.0f, eyeW = 1.0f;
	float focusX = 0.0f, focusY = 0.0f, focusZ = 0.0f, focusW = 1.0f;
	float upX = 0.0f, upY = 1.0f, upZ = 0.0f, upW = 0.0f;

	DirectX::XMFLOAT4 lightDirection = { 0.0f,0.0f,1.0f,0.0f };
	DirectX::XMFLOAT4 cameraPosition = { 0.0f,0.0f,0.0f,0.0f };

	DirectX::XMFLOAT3 translation = { 0,0,0 };
	DirectX::XMFLOAT3 scaling = { 1.0f,1.0f,1.0f };
	DirectX::XMFLOAT3 rotation = { 0,0,0 };
	DirectX::XMFLOAT4 materialColor = { 1,1,1,1 };

	// Animation関係
	int keyframeIndex = 0;
	DirectX::XMFLOAT4 setTestTranslation = { 0,0,0,0 };
	float factor = 0.5f;

	// 任意軸回転関係(Animation)
	DirectX::XMFLOAT4 rAxis = { 1,0,0,0 };
	float rotateAngle = 0;

	float luminanceMin = 0.6f;
	float luminanceMax = 0.8f;

	float blurGaussianSigma = 1.0f;
	float blurBloomIntensity = 1.0f;

	float toneExposure = 1.2f;

public:
	CONST HWND hwnd;

	enum class SAMPLER_STATE { POINT, LINEAR, ANISOTROPIC, LINEAR_BORDER_BLACK, LINEAR_BORDER_WHITE};
	Microsoft::WRL::ComPtr<ID3D11SamplerState> samplerStates[3];
	enum class DEPTH_STATE { ZT_ON_ZW_ON, ZT_ON_ZW_OFF, ZT_OFF_ZW_ON, ZT_OFF_ZW_OFF};
	Microsoft::WRL::ComPtr<ID3D11DepthStencilState> depthStencilStates[4]; // 描画順
	enum class BLEND_STATE { NONE, ALPHA, ADD, MULTIPRY};
	Microsoft::WRL::ComPtr<ID3D11BlendState> blendStates[4];
	enum class RASTER_STATE { SOLID, WIREFRAME, CULL_NONE, WIREFRAME_CULL_NONE};
	Microsoft::WRL::ComPtr<ID3D11RasterizerState> rasterizerStates[4];

	Microsoft::WRL::ComPtr<ID3D11Device> device; 
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> immediateContext;		// DX11の描画命令等を送るためのモノ
	Microsoft::WRL::ComPtr<IDXGISwapChain> swapChain;					// ハードウェアの情報が詰まっているもの
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> renderTargetView;	// ディスプレイのバックバッファのテクスチャを描画先として指定できるようにしたもの
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView> depthStencilView;
	std::unique_ptr<Sprite> sprites[8];
	std::unique_ptr<SpriteBatch> spriteBatches[8];

	struct SceneConstants {
		DirectX::XMFLOAT4X4 viewProjection;		// ビュー・プロジェクション変換行列
		DirectX::XMFLOAT4 lightDirection;		// ライトの向き
		DirectX::XMFLOAT4 cameraPosition;		// カメラの位置
	};
	Microsoft::WRL::ComPtr<ID3D11Buffer> constantBuffers[8];
	std::unique_ptr<GeometricPrimitive> geometricPrimitives[8];


	std::unique_ptr<StaticMesh> staticMeshes[8];

	std::unique_ptr<SkinnedMesh> skinnedMeshes[8];

	std::unique_ptr<Framebuffer> framebuffers[8];

	std::unique_ptr<FullscreenQuad> bitBlockTransfer;

	Microsoft::WRL::ComPtr<ID3D11PixelShader> pixelShaders[8];

	Microsoft::WRL::ComPtr<IXAudio2> xaudio2;
	IXAudio2MasteringVoice* masterVoice = nullptr;
	std::unique_ptr<Audio> bgm[8];
	std::unique_ptr<Audio> se[8];

	framework(HWND hwnd);
	~framework();

	framework(const framework&) = delete;
	framework& operator=(const framework&) = delete;
	framework(framework&&) noexcept = delete;
	framework& operator=(framework&&) noexcept = delete;

	int run()
	{
		MSG msg{};

		if (!initialize())
		{
			return 0;
		}

#ifdef USE_IMGUI
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGui::GetIO().Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\meiryo.ttc", 14.0f, nullptr, glyphRangesJapanese);
		ImGui_ImplWin32_Init(hwnd);
		ImGui_ImplDX11_Init(device.Get(), immediateContext.Get());
		ImGui::StyleColorsDark();
#endif

		while (WM_QUIT != msg.message)
		{
			if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
			else
			{
				tictoc.tick();
				calculate_frame_stats();
				update(tictoc.time_interval());
				render(tictoc.time_interval());
			}
		}

#ifdef USE_IMGUI
		ImGui_ImplDX11_Shutdown();
		ImGui_ImplWin32_Shutdown();
		ImGui::DestroyContext();
#endif

#if 1
		BOOL fullscreen = 0;
		swapChain->GetFullscreenState(&fullscreen, 0);
		if (fullscreen)
		{
			swapChain->SetFullscreenState(FALSE, 0);
		}
#endif

		return uninitialize() ? static_cast<int>(msg.wParam) : 0;
	}

	LRESULT CALLBACK handle_message(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
	{
#ifdef USE_IMGUI
		if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wparam, lparam)) { return true; }
#endif
		switch (msg)
		{
		case WM_PAINT:
		{
			PAINTSTRUCT ps{};
			BeginPaint(hwnd, &ps);

			EndPaint(hwnd, &ps);
		}
		break;

		case WM_DESTROY:
			PostQuitMessage(0);
			break;
		case WM_CREATE:
			break;
		case WM_KEYDOWN:
			if (wparam == VK_ESCAPE)
			{
				PostMessage(hwnd, WM_CLOSE, 0, 0);
			}
			break;
		case WM_ENTERSIZEMOVE:
			tictoc.stop();
			break;
		case WM_EXITSIZEMOVE:
			tictoc.start();
			break;
		default:
			return DefWindowProc(hwnd, msg, wparam, lparam);
		}
		return 0;
	}

private:
	bool initialize();
	void update(float elapsed_time/*Elapsed seconds from last frame*/);
	void render(float elapsed_time/*Elapsed seconds from last frame*/);
	bool uninitialize();

private:
	high_resolution_timer tictoc;
	uint32_t frames{ 0 };
	float elapsed_time{ 0.0f };
	void calculate_frame_stats()
	{
		if (++frames, (tictoc.time_stamp() - elapsed_time) >= 1.0f)
		{
			float fps = static_cast<float>(frames);
			std::wostringstream outs;
			outs.precision(6);
			outs << APPLICATION_NAME << L" : FPS : " << fps << L" / " << L"Frame Time : " << 1000.0f / fps << L" (ms)";
			SetWindowTextW(hwnd, outs.str().c_str());

			frames = 0;
			elapsed_time += 1.0f;
		}
	}
};

