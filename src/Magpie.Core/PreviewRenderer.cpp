#include "pch.h"
#include "include/PreviewRenderer.h"
#include "BackendDescriptorStore.h"
#include "DeviceResources.h"
#include "EffectDrawer.h"
#include "EffectCompiler.h"
#include "EffectsProfiler.h"
#include "Logger.h"
#include "StrHelper.h"
#include "TextureHelper.h"

namespace Magpie {

PreviewRenderer::PreviewRenderer() = default;
PreviewRenderer::~PreviewRenderer() = default;

static bool CompilePreviewEffect(
	const EffectOption& effect,
	bool disableEffectCache,
	bool noFP16,
	bool inlineParams,
	EffectDesc& desc
) noexcept {
	desc = { .name = effect.name };

	uint32_t compileFlags = 0;
	if (disableEffectCache) {
		compileFlags |= EffectCompilerFlags::NoCache;
	}
	if (noFP16) {
		compileFlags |= EffectCompilerFlags::NoFP16;
	}
	if (inlineParams) {
		compileFlags |= EffectCompilerFlags::InlineParams;
	}

	return EffectCompiler::Compile(desc, compileFlags, &effect.parameters) == 0;
}

bool PreviewRenderer::Initialize(
	const GraphicsCardId& graphicsCardId,
	CaptureMethod captureMethod
) noexcept {
	_deviceResources = std::make_unique<DeviceResources>();
	if (!_deviceResources->Initialize(false, &graphicsCardId, captureMethod)) {
		Logger::Get().Error("初始化预览渲染设备失败");
		_deviceResources.reset();
		return false;
	}

	_descriptorStore = std::make_unique<BackendDescriptorStore>();
	_descriptorStore->Initialize(_deviceResources->GetD3DDevice());
	return true;
}

bool PreviewRenderer::Render(
	const wchar_t* sampleImagePath,
	std::span<const EffectOption> effects,
	SIZE rendererSize,
	bool disableEffectCache,
	bool disableFP16,
	bool inlineParams,
	PreviewRenderResult& result,
	std::wstring* errorMessage
) noexcept {
	result = {};

	if (!_deviceResources || !_descriptorStore) {
		if (errorMessage) {
			*errorMessage = L"Preview renderer is not initialized.";
		}
		return false;
	}

	if (!sampleImagePath || !*sampleImagePath) {
		if (errorMessage) {
			*errorMessage = L"Preview sample image path is empty.";
		}
		return false;
	}

	winrt::com_ptr<ID3D11Texture2D> sourceTexture =
		TextureHelper::LoadTexture(sampleImagePath, _deviceResources->GetD3DDevice());
	if (!sourceTexture) {
		if (errorMessage) {
			*errorMessage = L"Failed to load preview sample image.";
		}
		return false;
	}

	const bool noFP16 = disableFP16 || !_deviceResources->IsFP16Supported();
	ID3D11Texture2D* outputTexture = sourceTexture.get();

	std::vector<EffectDesc> effectDescs;
	std::vector<EffectDrawer> effectDrawers;
	effectDescs.reserve(effects.size());
	effectDrawers.resize(effects.size());

	for (size_t i = 0; i < effects.size(); ++i) {
		EffectDesc& desc = effectDescs.emplace_back();
		if (!CompilePreviewEffect(effects[i], disableEffectCache, noFP16, inlineParams, desc)) {
			if (errorMessage) {
				*errorMessage = fmt::format(L"Failed to compile effect: {}", StrHelper::UTF8ToUTF16(effects[i].name));
			}
			return false;
		}

		if (!effectDrawers[i].Initialize(
			desc,
			effects[i],
			*_deviceResources,
			*_descriptorStore,
			&outputTexture,
			rendererSize,
			false
		)) {
			if (errorMessage) {
				*errorMessage = fmt::format(L"Failed to initialize effect: {}", StrHelper::UTF8ToUTF16(effects[i].name));
			}
			return false;
		}
	}

	ID3D11DeviceContext4* d3dDC = _deviceResources->GetD3DDC();
	d3dDC->ClearState();

	EffectsProfiler profiler;
	for (const EffectDrawer& drawer : effectDrawers) {
		drawer.Draw(profiler);
	}
	d3dDC->Flush();

	D3D11_TEXTURE2D_DESC desc{};
	outputTexture->GetDesc(&desc);
	if (desc.Format != DXGI_FORMAT_R8G8B8A8_UNORM && desc.Format != DXGI_FORMAT_B8G8R8A8_UNORM) {
		if (errorMessage) {
			*errorMessage = L"Preview renderer produced unsupported texture format.";
		}
		return false;
	}

	D3D11_TEXTURE2D_DESC stagingDesc = desc;
	stagingDesc.Usage = D3D11_USAGE_STAGING;
	stagingDesc.BindFlags = 0;
	stagingDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
	stagingDesc.MiscFlags = 0;

	winrt::com_ptr<ID3D11Texture2D> stagingTexture;
	HRESULT hr = _deviceResources->GetD3DDevice()->CreateTexture2D(
		&stagingDesc, nullptr, stagingTexture.put());
	if (FAILED(hr)) {
		Logger::Get().ComError("创建预览 staging 纹理失败", hr);
		if (errorMessage) {
			*errorMessage = L"Failed to create preview staging texture.";
		}
		return false;
	}

	d3dDC->CopyResource(stagingTexture.get(), outputTexture);

	D3D11_MAPPED_SUBRESOURCE mapped{};
	hr = d3dDC->Map(stagingTexture.get(), 0, D3D11_MAP_READ, 0, &mapped);
	if (FAILED(hr)) {
		Logger::Get().ComError("读取预览纹理失败", hr);
		if (errorMessage) {
			*errorMessage = L"Failed to read preview texture.";
		}
		return false;
	}

	result.width = desc.Width;
	result.height = desc.Height;
	result.rowPitch = desc.Width * 4;
	result.pixels.resize((size_t)result.rowPitch * desc.Height);

	const bool isRGBA = desc.Format == DXGI_FORMAT_R8G8B8A8_UNORM;
	for (uint32_t y = 0; y < desc.Height; ++y) {
		const uint8_t* srcRow = (const uint8_t*)mapped.pData + (size_t)y * mapped.RowPitch;
		uint8_t* dstRow = result.pixels.data() + (size_t)y * result.rowPitch;

		if (!isRGBA) {
			std::memcpy(dstRow, srcRow, result.rowPitch);
			continue;
		}

		for (uint32_t x = 0; x < desc.Width; ++x) {
			const size_t offset = (size_t)x * 4;
			dstRow[offset] = srcRow[offset + 2];
			dstRow[offset + 1] = srcRow[offset + 1];
			dstRow[offset + 2] = srcRow[offset];
			dstRow[offset + 3] = srcRow[offset + 3];
		}
	}

	d3dDC->Unmap(stagingTexture.get(), 0);
	return true;
}

}
