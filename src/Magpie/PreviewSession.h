#pragma once
#include "Event.h"
#include "ScalingPreviewService.h"
#include <memory>
#include <optional>
#include <functional>

namespace Magpie {

class PreviewSession : public std::enable_shared_from_this<PreviewSession> {
public:
	using ParamProvider = std::function<std::optional<PreviewRenderParams>()>;

	static std::shared_ptr<PreviewSession> Create(ParamProvider paramProvider);

	PreviewSession(const PreviewSession&) = delete;
	PreviewSession(PreviewSession&&) = delete;

	void RequestRefresh(bool immediate = false);
	void Cancel();

	winrt::Windows::UI::Xaml::Media::Imaging::SoftwareBitmapSource Image() const noexcept {
		return _image;
	}

	bool IsRendering() const noexcept {
		return _isRendering;
	}

	bool IsError() const noexcept {
		return _isError;
	}

	winrt::hstring ErrorText() const noexcept {
		return _errorText;
	}

	Event<const wchar_t*> PropertyChanged;

private:
	explicit PreviewSession(ParamProvider paramProvider);

	winrt::fire_and_forget _RefreshAsync(uint32_t requestId, bool immediate);

	void _ResetState() noexcept;
	void _SetImage(winrt::Windows::UI::Xaml::Media::Imaging::SoftwareBitmapSource image) noexcept;
	void _SetRendering(bool value) noexcept;
	void _SetError(bool value, winrt::hstring errorText = {}) noexcept;

	ParamProvider _paramProvider;
	winrt::Windows::UI::Xaml::Media::Imaging::SoftwareBitmapSource _image{ nullptr };
	winrt::hstring _errorText;
	bool _isRendering = false;
	bool _isError = false;
	bool _isRefreshPending = false;
	uint32_t _requestId = 0;
};

}
