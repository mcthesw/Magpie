#pragma once
#include "PreviewRenderer.h"
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

namespace Magpie {

struct PreviewRenderParams {
	std::vector<EffectOption> effects;
	std::optional<GraphicsCardId> graphicsCardId;
	SIZE renderSize{};
	bool disableEffectCache = false;
	bool disableFP16 = false;
	bool inlineParams = false;
};

class ScalingPreviewService {
public:
	static ScalingPreviewService& Get() noexcept {
		static ScalingPreviewService instance;
		return instance;
	}

	ScalingPreviewService(const ScalingPreviewService&) = delete;
	ScalingPreviewService(ScalingPreviewService&&) = delete;

	bool RenderPreview(
		const PreviewRenderParams& params,
		PreviewRenderResult& renderResult,
		std::wstring& errorMessage
	) noexcept;

private:
	ScalingPreviewService() = default;

	static bool _IsSameGraphicsCardId(const GraphicsCardId& l, const GraphicsCardId& r) noexcept;
	std::filesystem::path _ResolveSampleImagePath() noexcept;

	std::mutex _mutex;
	std::unique_ptr<PreviewRenderer> _renderer;
	GraphicsCardId _rendererGraphicsCardId{};
	bool _hasRenderer = false;
};

}
