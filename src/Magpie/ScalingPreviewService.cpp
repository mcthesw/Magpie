#include "pch.h"
#include "ScalingPreviewService.h"
#include "Win32Helper.h"

namespace Magpie {

bool ScalingPreviewService::_IsSameGraphicsCardId(const GraphicsCardId& l, const GraphicsCardId& r) noexcept {
	return l.idx == r.idx && l.vendorId == r.vendorId && l.deviceId == r.deviceId;
}

std::filesystem::path ScalingPreviewService::_ResolveSampleImagePath() noexcept {

	const std::filesystem::path exeDir = Win32Helper::GetExePath().parent_path();

	const std::filesystem::path candidates[] = {
		exeDir / L"assets" / L"preview" / L"repo-card.png",
#ifdef _DEBUG
		exeDir / L"repo-card.png",
		exeDir / L"img" / L"repo-card.png",
		exeDir / L".." / L".." / L".." / L"img" / L"repo-card.png",
		exeDir / L".." / L".." / L".." / L".." / L"img" / L"repo-card.png",
#endif
	};

	for (const std::filesystem::path& path : candidates) {
		const std::filesystem::path fullPath = path.lexically_normal();
		if (Win32Helper::FileExists(fullPath.c_str())) {
			return fullPath;
		}
	}

	return {};
}

bool ScalingPreviewService::RenderPreview(
	const PreviewRenderParams& params,
	PreviewRenderResult& renderResult,
	std::wstring& errorMessage
) noexcept {
	std::scoped_lock lock(_mutex);

	renderResult = {};
	errorMessage.clear();

	const std::filesystem::path sampleImagePath = _ResolveSampleImagePath();
	if (sampleImagePath.empty()) {
		errorMessage = L"Preview sample image was not found.";
		return false;
	}

	const GraphicsCardId cardId = params.graphicsCardId.has_value() ? *params.graphicsCardId : GraphicsCardId{};
	if (!_hasRenderer || !_renderer || !_IsSameGraphicsCardId(_rendererGraphicsCardId, cardId)) {
		_renderer = std::make_unique<PreviewRenderer>();
		if (!_renderer->Initialize(cardId)) {
			_renderer.reset();
			_hasRenderer = false;
			errorMessage = L"Failed to initialize preview renderer.";
			return false;
		}

		_rendererGraphicsCardId = cardId;
		_hasRenderer = true;
	}

	PreviewRenderResult newRenderResult;
	if (!_renderer->Render(
		sampleImagePath.c_str(),
		params.effects,
		params.renderSize,
		params.disableEffectCache,
		params.disableFP16,
		params.inlineParams,
		newRenderResult,
		&errorMessage
	)) {
		return false;
	}

	if (newRenderResult.width == 0 || newRenderResult.height == 0 || newRenderResult.pixels.empty()) {
		errorMessage = L"Preview renderer produced empty output.";
		return false;
	}

	renderResult = std::move(newRenderResult);
	return true;
}

}
