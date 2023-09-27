#include "framework.h"

framework::framework(HWND hwnd) : hwnd(hwnd)
{	
}

bool framework::initialize()
{
	// デバイス・デバイスコンテキスト・スワップチェーンの作成
	HRESULT hr = S_OK;

	UINT createDeviceFlags = 0;
#ifdef _DEBUG
	createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	D3D_FEATURE_LEVEL featureLevels = D3D_FEATURE_LEVEL_11_0;

	DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
	swapChainDesc.BufferCount = 1;
	swapChainDesc.BufferDesc.Width = SCREEN_WIDTH;					// バッファの横幅
	swapChainDesc.BufferDesc.Height = SCREEN_HEIGHT;				// バッファの縦幅
	swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;	// カラーフォーマット
	swapChainDesc.BufferDesc.RefreshRate.Numerator = 60;			// リフレッシュレートの分母
	swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;			// リフレッシュレートの分子
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;	// バッファの使い方 Usage => 使用方法
	swapChainDesc.OutputWindow = hwnd;								// 出力するウィンドウのハンドル
	swapChainDesc.SampleDesc.Count = 1;								// マルチサンプリングのサンプル数(未使用は1)
	swapChainDesc.SampleDesc.Quality = 0;							// マルチサンプリングの品質(未使用は0)
	swapChainDesc.Windowed = !FULLSCREEN;							// フルスクリーン設定
	hr = D3D11CreateDeviceAndSwapChain(
		NULL,
		D3D_DRIVER_TYPE_HARDWARE,
		NULL,								// ビデオアダプタ指定(既定はnullptr)
		createDeviceFlags,					// ドライバのタイプ
		&featureLevels,						// GPUに応じたDirectXの機能レベル(今回は11)
		1,									// D3D_FEATURE_LEVEL配列の要素数
		D3D11_SDK_VERSION,					// SDKバージョン
		&swapChainDesc,						// DXGI_SWAP_CHAIN_DESC
		&swapChain,							// 関数成功時のSwapChainの出力先
		&device,							// 関数成功時のDeviceの出力先
		NULL,								// 成功したD3D_FEATURE_LEVELの出力先
		&immediateContext					// 関数成功時のContextの出力先 // ContextはCPU側で追加された描画コマンドをGPU側に送信するのが主な役目
	);
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr)); 

	// レンダーターゲットビューの作成
	ID3D11Texture2D* backBuffer = {};
	hr = swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<LPVOID*>(&backBuffer));
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

	hr = device->CreateRenderTargetView(backBuffer, NULL, renderTargetView.GetAddressOf());
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

	backBuffer->Release(); // backBufferの解放

	// 深度ステンシルビューの作成
	ID3D11Texture2D* depthStencilBuffer = {};
	D3D11_TEXTURE2D_DESC texture2dDesc = {};
	texture2dDesc.Width = SCREEN_WIDTH;
	texture2dDesc.Height = SCREEN_HEIGHT;
	texture2dDesc.MipLevels = 1;
	texture2dDesc.ArraySize = 1;
	texture2dDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	texture2dDesc.SampleDesc.Count = 1;
	texture2dDesc.SampleDesc.Quality = 0;
	texture2dDesc.Usage = D3D11_USAGE_DEFAULT;
	texture2dDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	texture2dDesc.CPUAccessFlags = 0;
	texture2dDesc.MiscFlags = 0;
	hr = device->CreateTexture2D(&texture2dDesc, NULL, &depthStencilBuffer);
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

	D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc = {};
	depthStencilViewDesc.Format = texture2dDesc.Format;
	depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	depthStencilViewDesc.Texture2D.MipSlice = 0;
	hr = device->CreateDepthStencilView(depthStencilBuffer, &depthStencilViewDesc, &depthStencilView);
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

	depthStencilBuffer->Release();

	// ビューポートの設定
	D3D11_VIEWPORT viewport = {};
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.Width = static_cast<float>(SCREEN_WIDTH);
	viewport.Height = static_cast<float>(SCREEN_HEIGHT);
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;
	immediateContext->RSSetViewports(1, &viewport);

	// サンプラーステートオブジェクトの生成
	D3D11_SAMPLER_DESC samplerDesc;
	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_BORDER; // サンプリング時のUVの値が範囲外の時どうするかを指定
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_BORDER; // Uは横、Vは縦、Wは奥行き
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
	samplerDesc.MipLODBias = 0;
	samplerDesc.MaxAnisotropy = 16;
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
	samplerDesc.BorderColor[0] = 0;
	samplerDesc.BorderColor[1] = 0;
	samplerDesc.BorderColor[2] = 0;
	samplerDesc.BorderColor[3] = 1;
	samplerDesc.MinLOD = 0;
	samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
	hr = device->CreateSamplerState(&samplerDesc, samplerStates[static_cast<size_t>(SAMPLER_STATE::POINT)].GetAddressOf());
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	hr = device->CreateSamplerState(&samplerDesc, samplerStates[static_cast<size_t>(SAMPLER_STATE::LINEAR)].GetAddressOf());
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

	samplerDesc.Filter = D3D11_FILTER_ANISOTROPIC;
	hr = device->CreateSamplerState(&samplerDesc, samplerStates[static_cast<size_t>(SAMPLER_STATE::ANISOTROPIC)].GetAddressOf());
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

	// 深度ステンシルステートオブジェクトの作成
	// 深度テスト：オン 深度ライト：オン
	D3D11_DEPTH_STENCIL_DESC depthStencilDesc = {};
	depthStencilDesc.DepthEnable = TRUE;
	depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	depthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
	hr = device->CreateDepthStencilState(&depthStencilDesc, depthStencilStates[static_cast<size_t>(DEPTH_STATE::ZT_ON_ZW_ON)].GetAddressOf());
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));
	// 深度テスト：オン 深度ライト：オフ
	depthStencilDesc = {};
	depthStencilDesc.DepthEnable = TRUE;
	depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	depthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
	hr = device->CreateDepthStencilState(&depthStencilDesc, depthStencilStates[static_cast<size_t>(DEPTH_STATE::ZT_ON_ZW_OFF)].GetAddressOf());
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));
	// 深度テスト：オフ 深度ライト：オン
	depthStencilDesc = {};
	depthStencilDesc.DepthEnable = FALSE;
	depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	depthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
	hr = device->CreateDepthStencilState(&depthStencilDesc, depthStencilStates[static_cast<size_t>(DEPTH_STATE::ZT_OFF_ZW_ON)].GetAddressOf());
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));
	// 深度テスト：オフ 深度ライト：オフ
	depthStencilDesc = {};
	depthStencilDesc.DepthEnable = FALSE;
	depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	depthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
	hr = device->CreateDepthStencilState(&depthStencilDesc, depthStencilStates[static_cast<size_t>(DEPTH_STATE::ZT_OFF_ZW_OFF)].GetAddressOf());
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

	// ブレンディングステートオブジェクトの作成
	// dest = 元の画像の色, src = 今から付ける色
	D3D11_BLEND_DESC blendDesc = {};
	blendDesc.AlphaToCoverageEnable = FALSE;
	blendDesc.IndependentBlendEnable = FALSE;
	blendDesc.RenderTarget[0].BlendEnable = TRUE; // RenderTargetの配列は8個あるが、setBlendStateではRenderTarget配列の先頭(0)しか参照しない
	blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;						// SrcBlend * SRC_ALPHA
	blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;				// DestBlend * 1 - SRC_ALPHA
	blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;							// SrcBlendとDestBlendを足す
	blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;						// SrcBlendAlpha * 0 
	blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;					// DestBlendAlpha * 1 - SRC_ALPHA
	blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;					// SrcBlendAlphaとDestBlendAlphaを足す
	blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL; // MaskにR,G,B,Aを格納 Maskについてはここを見ろ→https://qiita.com/drken/items/7c6ff2aa4d8fce1c9361#4-%E3%83%9E%E3%82%B9%E3%82%AF%E3%83%93%E3%83%83%E3%83%88
	hr = device->CreateBlendState(&blendDesc, blendStates[static_cast<size_t>(BLEND_STATE::NONE)].GetAddressOf());
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));
	// 透過
	blendDesc = {};
	blendDesc.AlphaToCoverageEnable = FALSE;
	blendDesc.IndependentBlendEnable = FALSE;
	blendDesc.RenderTarget[0].BlendEnable = TRUE; // RenderTargetの配列は8個あるが、setBlendStateではRenderTarget配列の先頭(0)しか参照しない
	blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;						// SrcBlend * SRC_ALPHA
	blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;				// DestBlend * 1 - SRC_ALPHA
	blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;							// SrcBlendとDestBlendを足す
	blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ZERO;						// SrcBlendAlpha * 0 
	blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;			// DestBlendAlpha * 1 - SRC_ALPHA
	blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;					// SrcBlendAlphaとDestBlendAlphaを足す
	blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL; // MaskにR,G,B,Aを格納 Maskについてはここを見ろ→https://qiita.com/drken/items/7c6ff2aa4d8fce1c9361#4-%E3%83%9E%E3%82%B9%E3%82%AF%E3%83%93%E3%83%83%E3%83%88
	hr = device->CreateBlendState(&blendDesc, blendStates[static_cast<size_t>(BLEND_STATE::ALPHA)].GetAddressOf());
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));
	// 加算
	blendDesc = {};
	blendDesc.AlphaToCoverageEnable = FALSE;
	blendDesc.IndependentBlendEnable = FALSE;
	blendDesc.RenderTarget[0].BlendEnable = TRUE;
	blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
	blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ZERO;
	blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
	blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
	hr = device->CreateBlendState(&blendDesc, blendStates[static_cast<size_t>(BLEND_STATE::ADD)].GetAddressOf());
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));
	// 乗算
	blendDesc = {};
	blendDesc.AlphaToCoverageEnable = FALSE;
	blendDesc.IndependentBlendEnable = FALSE;
	blendDesc.RenderTarget[0].BlendEnable = TRUE;
	blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_ZERO;
	blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_SRC_COLOR;
	blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_DEST_ALPHA;
	blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
	blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
	hr = device->CreateBlendState(&blendDesc, blendStates[static_cast<size_t>(BLEND_STATE::MULTIPRY)].GetAddressOf());
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

	// シーン定数バッファオブジェクトの生成

	D3D11_BUFFER_DESC bufferDesc = {};
	bufferDesc.ByteWidth = sizeof(SceneConstants);
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bufferDesc.CPUAccessFlags = 0;
	bufferDesc.MiscFlags = 0;
	bufferDesc.StructureByteStride = 0;
	hr = device->CreateBuffer(&bufferDesc, nullptr, constantBuffers[0].GetAddressOf());
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

	// ラスタライザステートオブジェクトの作成
	D3D11_RASTERIZER_DESC rasterizerDesc = {};
	rasterizerDesc.FillMode = D3D11_FILL_SOLID;			// 頂点によって生成された三角形を塗りつぶす
	rasterizerDesc.CullMode = D3D11_CULL_BACK;			// 法線に沿って描画する
	rasterizerDesc.FrontCounterClockwise = TRUE;		// 三角形の表面を決める TRUEなら反時計回り、FALSEなら時計回り
	rasterizerDesc.DepthBias = 0;						// 深度バイアスを計算するときに使う値 同じ深度値があった場合に優先順位を付けるためのもの
	rasterizerDesc.DepthBiasClamp = 0;					// ↑と同じ
	rasterizerDesc.SlopeScaledDepthBias = 0;			// ↑と同じ
	rasterizerDesc.DepthClipEnable = TRUE;				// 距離によるクリッピングを行うか
	rasterizerDesc.ScissorEnable = FALSE;				// シザー矩形カリングを行うかのフラグ シザー矩形について https://tositeru.github.io/ImasaraDX11/part/rasterizer-state
	rasterizerDesc.MultisampleEnable = FALSE;			// MSAAのレンダーターゲットを使用時、四辺形ラインアンチエイリアスを行うか、アルファラインアンチエイリアスをするか決めるフラグ
	rasterizerDesc.AntialiasedLineEnable = FALSE;		// MSAAのレンダーターゲットを使用時、線分描画で↑がFALSEの時、アンチエイリアスを有効にする
	hr = device->CreateRasterizerState(&rasterizerDesc, rasterizerStates[static_cast<size_t>(RASTER_STATE::SOLID)].GetAddressOf());
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

	rasterizerDesc.FillMode = D3D11_FILL_WIREFRAME;		// 頂点を結ぶ線を描画する
	rasterizerDesc.CullMode = D3D11_CULL_BACK;			// 法線に沿って描画する
	rasterizerDesc.AntialiasedLineEnable = TRUE;
	hr = device->CreateRasterizerState(&rasterizerDesc, rasterizerStates[static_cast<size_t>(RASTER_STATE::WIREFRAME)].GetAddressOf());
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

	rasterizerDesc.FillMode = D3D11_FILL_SOLID;
	rasterizerDesc.CullMode = D3D11_CULL_NONE;			// 法線に沿って描画しない
	rasterizerDesc.AntialiasedLineEnable = TRUE;
	hr = device->CreateRasterizerState(&rasterizerDesc, rasterizerStates[static_cast<size_t>(RASTER_STATE::CULL_NONE)].GetAddressOf());
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

	rasterizerDesc.FillMode = D3D11_FILL_WIREFRAME;
	rasterizerDesc.CullMode = D3D11_CULL_NONE;			// 法線に沿って描画しない
	rasterizerDesc.AntialiasedLineEnable = TRUE;
	hr = device->CreateRasterizerState(&rasterizerDesc, rasterizerStates[static_cast<size_t>(RASTER_STATE::WIREFRAME_CULL_NONE)].GetAddressOf());
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

	// スタティックメッシュ生成
	//staticMeshes[0] = std::make_unique<StaticMesh>(device.Get(), L".\\resources\\Cup\\cup.obj");
	//staticMeshes[1] = std::make_unique<StaticMesh>(device.Get(), L".\\resources\\Rock\\rock.obj",true);

	// スキンドメッシュの生成
	spriteBatches[0] = std::make_unique<SpriteBatch>(device.Get(), L".\\resources\\screenshot.jpg", 1);
	skinnedMeshes[0] = std::make_unique<SkinnedMesh>(device.Get(), ".\\resources\\nico.fbx");

	// framebufferオブジェクトの生成
	framebuffers[0] = std::make_unique<Framebuffer>(device.Get(), 1280, 720);
	framebuffers[1] = std::make_unique<Framebuffer>(device.Get(), 1280 / 2, 720 / 2);

	// fullscreenQuadオブジェクトの生成
	bitBlockTransfer = std::make_unique<FullscreenQuad>(device.Get());

	create_ps_from_cso(device.Get(), "luminance_extraction_ps.cso", pixelShaders[0].GetAddressOf());
	create_ps_from_cso(device.Get(), "blur_ps.cso", pixelShaders[1].GetAddressOf());

	hr = XAudio2Create(xaudio2.GetAddressOf(), 0, XAUDIO2_DEFAULT_PROCESSOR);
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

	hr = xaudio2->CreateMasteringVoice(&masterVoice);
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

	se[0] = std::make_unique<Audio>(xaudio2.Get(), L".\\resources\\打撃2.wav");

	return true;
}

void framework::update(float elapsed_time/*Elapsed seconds from last frame*/)
{
#ifdef USE_IMGUI
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
#endif
	
#ifdef USE_IMGUI
	ImGui::Begin("ImGui");
	if (ImGui::TreeNode(u8"カメラとライト")) {
		if (ImGui::TreeNode("Light")) {
			ImGui::SliderFloat4("LightDirection", &lightDirection.x, -1.0f, 1.0f);
			ImGui::TreePop();
		}
		if (ImGui::TreeNode("Camera")) {
			ImGui::SliderFloat4("eye",	 &eyeX,	  -100.0f, +100.0f);
			ImGui::SliderFloat4("focus", &focusX, -100.0f, +100.0f);
			ImGui::SliderFloat4("up",    &upX,	  -100.0f, +100.0f);

			ImGui::TreePop();
		}
		if (ImGui::TreeNode(u8"輝度")) {
			ImGui::InputFloat("Min", &luminanceMin);
			ImGui::InputFloat("Max", &luminanceMax);

			ImGui::TreePop();
		}
		if (ImGui::TreeNode(u8"ブルーム")) {
			ImGui::InputFloat("BlurGaussianSigma", &blurGaussianSigma);
			ImGui::InputFloat("BlurBloomIntensity", &blurBloomIntensity);

			ImGui::TreePop();
		}
		if (ImGui::TreeNode(u8"トーンマッピング")) {
			ImGui::InputFloat("ToneExposure", &toneExposure);

			ImGui::TreePop();
		}
		ImGui::TreePop();
	}
	if (ImGui::TreeNode(u8"モデル")) {
		if (ImGui::TreeNode("Transform")) {
			ImGui::SliderFloat3("position", &translation.x, -10.0f, +10.0f);
			ImGui::InputFloat3("scale",    &scaling.x, -10.0f, +10.0f);
			ImGui::SliderFloat3("rotate",   &rotation.x,  -10.0f, +10.0f);
			ImGui::TreePop();
		}
		if (ImGui::TreeNode("Color")) {
			ImGui::ColorEdit4("color", &materialColor.x);
			ImGui::TreePop();
		}
		if (ImGui::TreeNode("Animation")) {
			ImGui::InputInt("keyFrameIndex", &keyframeIndex);
			ImGui::InputFloat4("setTestTranslation", &setTestTranslation.x);

			ImGui::InputFloat4("rAxis", &rAxis.x);
			ImGui::InputFloat("rotateAngle", &rotateAngle);
			ImGui::TreePop();
		}
		ImGui::TreePop();
	}
	ImGui::End();
#endif

	if (GetKeyState('W') & 0x8000) {
		se[0]->play();
	}
	/*else { // ここでelseすると押し続けないと再生しきらない
		se[0]->stop();
	}*/
}
void framework::render(float elapsed_time/*Elapsed seconds from last frame*/)
{
	HRESULT hr = S_OK;

	ID3D11RenderTargetView* nullRTViews[D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT] = {};
	immediateContext.Get()->OMSetRenderTargets(_countof(nullRTViews), nullRTViews, 0);
	ID3D11ShaderResourceView* nullSRViews[D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT] = {};
	immediateContext.Get()->VSSetShaderResources(0, _countof(nullSRViews), nullSRViews);
	immediateContext.Get()->PSSetShaderResources(0, _countof(nullSRViews), nullSRViews);

	FLOAT color[]{ 0.0f,0.5f,0.2f,1.0f };

	immediateContext.Get()->ClearRenderTargetView(renderTargetView.Get(), color);
	immediateContext.Get()->ClearDepthStencilView(depthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	immediateContext.Get()->OMSetRenderTargets(1, renderTargetView.GetAddressOf(), depthStencilView.Get());

	immediateContext.Get()->OMSetDepthStencilState(depthStencilStates[static_cast<size_t>(DEPTH_STATE::ZT_OFF_ZW_OFF)].Get(), 1);

	immediateContext.Get()->OMSetBlendState(blendStates[static_cast<size_t>(BLEND_STATE::ALPHA)].Get(), nullptr, 0xffffffff);

	immediateContext.Get()->PSSetSamplers(0, 1, samplerStates[0].GetAddressOf());
	immediateContext.Get()->PSSetSamplers(1, 1, samplerStates[1].GetAddressOf());
	immediateContext.Get()->PSSetSamplers(2, 1, samplerStates[2].GetAddressOf());

	immediateContext.Get()->RSSetState(rasterizerStates[static_cast<size_t>(RASTER_STATE::SOLID)].Get());

	// ビュー・プロジェクション変換行列を計算し、定数バッファにセット
	D3D11_VIEWPORT viewport;
	UINT numViewports = 1;
	immediateContext->RSGetViewports(&numViewports, &viewport);

	float aspectRatio = viewport.Width / viewport.Height;
	DirectX::XMMATRIX P = DirectX::XMMatrixPerspectiveFovLH(DirectX::XMConvertToRadians(30), aspectRatio, 0.1f/*near panel*/, 100.0f/*far panel*/);

	DirectX::XMVECTOR eye = DirectX::XMVectorSet(eyeX, eyeY, eyeZ, eyeW);
	DirectX::XMVECTOR focus = DirectX::XMVectorSet(focusX, focusY, focusZ, focusW);
	DirectX::XMVECTOR up = DirectX::XMVectorSet(upX, upY, upZ, upW);
	DirectX::XMMATRIX V = DirectX::XMMatrixLookAtLH(eye, focus, up);

	SceneConstants data = {};
	DirectX::XMStoreFloat4x4(&data.viewProjection, V * P);
	data.lightDirection = lightDirection;
	data.cameraPosition = cameraPosition;

	immediateContext.Get()->UpdateSubresource(constantBuffers[0].Get(), 0, 0, &data, 0, 0);
	immediateContext.Get()->VSSetConstantBuffers(1, 1, constantBuffers[0].GetAddressOf());
	immediateContext.Get()->PSSetConstantBuffers(1, 1, constantBuffers[0].GetAddressOf());

	const DirectX::XMFLOAT4X4 coordinateSystemTransforms[] = {
		{-1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1},					// 0:RHS Y-UP
		{1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1},					// 1:LHS Y-UP
		{-1,0,0,0,0,0,-1,0,0,1,0,0,0,0,0,1},				// 2:RHS Z-UP
		{1,0,0,0,0,0,1,0,0,1,0,0,0,0,0,1},					// 3:LHS Z-UP
	};
	// To change the units from centimeters to meters, set 'scaleFactor' to 0.01.
#if 0
	const float scaleFactor = 1.0f;
#else
	const float scaleFactor = 0.01f;
#endif

	DirectX::XMMATRIX C = DirectX::XMLoadFloat4x4(&coordinateSystemTransforms[0])
		* DirectX::XMMatrixScaling(scaleFactor, scaleFactor, scaleFactor);

	DirectX::XMMATRIX S = DirectX::XMMatrixScaling(scaling.x, scaling.y, scaling.z);
	DirectX::XMMATRIX R = DirectX::XMMatrixRotationRollPitchYaw(rotation.x, rotation.y, rotation.z);
	DirectX::XMMATRIX T = DirectX::XMMatrixTranslation(translation.x, translation.y, translation.z);
	DirectX::XMFLOAT4X4 world;
	DirectX::XMStoreFloat4x4(&world, C * S * R * T);

	framebuffers[0]->clear(immediateContext.Get());
	framebuffers[0]->activate(immediateContext.Get());

	immediateContext.Get()->RSSetState(rasterizerStates[static_cast<size_t>(RASTER_STATE::CULL_NONE)].Get());
	immediateContext.Get()->OMSetDepthStencilState(depthStencilStates[static_cast<size_t>(DEPTH_STATE::ZT_OFF_ZW_OFF)].Get(), 0);
	immediateContext.Get()->OMSetBlendState(blendStates[static_cast<size_t>(BLEND_STATE::NONE)].Get(), nullptr, 0xffffffff);
	spriteBatches[0]->begin(immediateContext.Get());
	spriteBatches[0]->render(immediateContext.Get(), 0, 0, 1280, 720);
	spriteBatches[0]->end(immediateContext.Get());

	immediateContext.Get()->OMSetDepthStencilState(depthStencilStates[static_cast<size_t>(DEPTH_STATE::ZT_ON_ZW_ON)].Get(), 0);
	immediateContext.Get()->RSSetState(rasterizerStates[static_cast<size_t>(RASTER_STATE::SOLID)].Get());
	immediateContext.Get()->OMSetBlendState(blendStates[static_cast<size_t>(BLEND_STATE::NONE)].Get(), nullptr, 0xffffffff);

	if (skinnedMeshes[0]->animationClips.size() > 0) {
#if 1
		int clipIndex = 0;
		int frameIndex = 0;
		static float animationTick = 0;

		SkinnedMesh::Animation& animation = skinnedMeshes[0]->animationClips.at(clipIndex);
		frameIndex = static_cast<int>(animationTick * animation.samplingRate);
		if (frameIndex > animation.sequence.size() - 1) {
			frameIndex = 0;
			animationTick = 0;
		}
		else {
			animationTick += elapsed_time;
		}

		SkinnedMesh::Animation::Keyframe& keyframe = animation.sequence.at(frameIndex);
#else
		SkinnedMesh::Animation::Keyframe keyframe;
		const SkinnedMesh::Animation::Keyframe* keyframes[2] = {
			&skinnedMeshes[0]->animationClips.at(0).sequence.at(40),
			&skinnedMeshes[0]->animationClips.at(0).sequence.at(80),
		};

		skinnedMeshes[0]->blend_animations(keyframes, factor, keyframe);
		skinnedMeshes[0]->update_animation(keyframe);
#endif

#if 0
		XMStoreFloat4(&keyframe.nodes.at(keyframeIndex).rotation,
			DirectX::XMQuaternionRotationAxis(DirectX::XMVectorSet(rAxis.x, rAxis.y, rAxis.z, rAxis.w), rotateAngle));
		keyframe.nodes.at(keyframeIndex).translation.x = setTestTranslation.x;
		skinnedMeshes[0]->update_animation(keyframe);
#endif
		skinnedMeshes[0]->render(immediateContext.Get(), world, materialColor, &keyframe);
	}
	else {
		skinnedMeshes[0]->render(immediateContext.Get(), world, materialColor, nullptr);
	}

	framebuffers[0]->deactivate(immediateContext.Get());

#if 1
	immediateContext.Get()->RSSetState(rasterizerStates[static_cast<size_t>(RASTER_STATE::CULL_NONE)].Get());
	immediateContext.Get()->OMSetDepthStencilState(depthStencilStates[static_cast<size_t>(DEPTH_STATE::ZT_OFF_ZW_OFF)].Get(), 0);
	bitBlockTransfer->blit(immediateContext.Get(), framebuffers[0]->shaderResourceViews[0].GetAddressOf(), 0, 1);
#endif

	framebuffers[1]->clear(immediateContext.Get());
	framebuffers[1]->activate(immediateContext.Get());
	bitBlockTransfer->blit(immediateContext.Get(),
		framebuffers[0]->shaderResourceViews[0].GetAddressOf(), 0, 1, pixelShaders[0].Get());
	framebuffers[1]->deactivate(immediateContext.Get());

#if 1
	bitBlockTransfer->set_luminance_clamp(immediateContext.Get(), luminanceMin, luminanceMax);
	bitBlockTransfer->blit(immediateContext.Get(),
		framebuffers[1]->shaderResourceViews[0].GetAddressOf(), 0, 1);
#endif

	ID3D11ShaderResourceView* shaderResourceViews[2] = {
		framebuffers[0]->shaderResourceViews[0].Get(),framebuffers[1]->shaderResourceViews->Get(),
	};

	bitBlockTransfer->set_blur(immediateContext.Get(), blurGaussianSigma, blurBloomIntensity);
	bitBlockTransfer->set_tone_exposure(immediateContext.Get(), toneExposure);
	bitBlockTransfer->blit(immediateContext.Get(), shaderResourceViews, 0, 2,pixelShaders[1].Get());


#ifdef USE_IMGUI
	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
#endif

	UINT syncInterval = 0;
	
	swapChain->Present(syncInterval, 0);
	
}

bool framework::uninitialize()
{
	/*device->Release();
	immediateContext->Release();
	swapChain->Release();
	renderTargetView->Release();
	depthStencilView->Release();*/
	//for (Sprite* p : sprites) delete p;
	//for (ID3D11SamplerState* p : samplerStates) p->Release();
	//for (ID3D11DepthStencilState* p : depthStencilStates) p->Release();
	//for (ID3D11BlendState* p : blendStates) p->Release();
	//for (SpriteBatch* p : spriteBatches) delete p;
	return true;
}

framework::~framework()
{

}