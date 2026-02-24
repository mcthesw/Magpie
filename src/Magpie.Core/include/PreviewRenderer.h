#pragma once
#include "ScalingOptions.h"
#include <optional>
#include <span>
#include <string>
#include <vector>
#include <memory>

namespace Magpie {

struct PreviewRenderResult {
	uint32_t width = 0;
	uint32_t height = 0;
	uint32_t rowPitch = 0;
	// BGRA8，行跨度为 rowPitch
	std::vector<uint8_t> pixels;
};

class PreviewRenderer {
public:
	PreviewRenderer();
	~PreviewRenderer();
	PreviewRenderer(const PreviewRenderer&) = delete;
	PreviewRenderer(PreviewRenderer&&) = delete;

	bool Initialize(
		const GraphicsCardId& graphicsCardId = {},
		CaptureMethod captureMethod = CaptureMethod::GDI
	) noexcept;

	bool Render(
		const wchar_t* sampleImagePath,
		std::span<const EffectOption> effects,
		SIZE rendererSize,
		bool disableEffectCache,
		bool disableFP16,
		bool inlineParams,
		PreviewRenderResult& result,
		std::wstring* errorMessage = nullptr
	) noexcept;

private:
	std::unique_ptr<class DeviceResources> _deviceResources;
	std::unique_ptr<class BackendDescriptorStore> _descriptorStore;
};

}
