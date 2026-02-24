#include "pch.h"
#include "PreviewSession.h"
#include "App.h"
#include "CommonSharedConstants.h"
#include "Logger.h"
#include <winrt/Windows.Graphics.Imaging.h>

using namespace winrt;
using namespace Windows::Graphics::Imaging;
using namespace Windows::UI::Xaml::Media::Imaging;
using App = winrt::Magpie::implementation::App;

namespace Magpie {

std::shared_ptr<PreviewSession> PreviewSession::Create(ParamProvider paramProvider) {
	return std::shared_ptr<PreviewSession>(new PreviewSession(std::move(paramProvider)));
}

PreviewSession::PreviewSession(ParamProvider paramProvider) :
	_paramProvider(std::move(paramProvider)) {}

void PreviewSession::RequestRefresh(bool immediate) {
	if (_isRendering) {
		_isRefreshPending = true;
		return;
	}

	_RefreshAsync(++_requestId, immediate);
}

void PreviewSession::Cancel() {
	++_requestId;
	_isRefreshPending = false;
	_ResetState();
}

void PreviewSession::_ResetState() noexcept {
	_SetRendering(false);
	_SetError(false);
	_SetImage(nullptr);
}

void PreviewSession::_SetImage(SoftwareBitmapSource image) noexcept {
	if (_image == image) {
		return;
	}

	_image = std::move(image);
	PropertyChanged.Invoke(L"PreviewImage");
}

void PreviewSession::_SetRendering(bool value) noexcept {
	if (_isRendering == value) {
		return;
	}

	_isRendering = value;
	PropertyChanged.Invoke(L"IsPreviewRendering");
}

void PreviewSession::_SetError(bool value, hstring errorText) noexcept {
	if (_isError != value) {
		_isError = value;
		PropertyChanged.Invoke(L"IsPreviewError");
	}

	if (_errorText != errorText) {
		_errorText = std::move(errorText);
		PropertyChanged.Invoke(L"PreviewErrorText");
	}
}

fire_and_forget PreviewSession::_RefreshAsync(uint32_t requestId, bool immediate) {
	auto weakThis = weak_from_this();
	std::optional<winrt::apartment_context> ui;
	bool hasException = false;
	hstring exceptionMessage;

	try {
		if (!immediate) {
			co_await std::chrono::milliseconds(120);
		}

		co_await App::Get().Dispatcher();
		ui.emplace();

		auto that = weakThis.lock();
		if (!that || that->_requestId != requestId) {
			co_return;
		}

		std::optional<PreviewRenderParams> params = that->_paramProvider ? that->_paramProvider() : std::nullopt;
		if (!params.has_value()) {
			that->_ResetState();
			co_return;
		}

		that->_SetRendering(true);
		that->_SetError(false);

		std::wstring errorMessage;
		PreviewRenderResult previewResult;

		co_await resume_background();
		const bool hasPreviewResult = ScalingPreviewService::Get().RenderPreview(
			*params,
			previewResult,
			errorMessage
		);

		co_await *ui;

		that = weakThis.lock();
		if (!that || that->_requestId != requestId) {
			co_return;
		}

		that->_SetRendering(false);

		if (!hasPreviewResult || previewResult.width == 0 || previewResult.height == 0 || previewResult.pixels.empty()) {
			hstring finalError;
			if (errorMessage.empty()) {
				const ResourceLoader resourceLoader =
					ResourceLoader::GetForCurrentView(CommonSharedConstants::APP_RESOURCE_MAP_ID);
				finalError = resourceLoader.GetString(L"Preview_RenderFailed/Text");
			} else {
				finalError = hstring(errorMessage);
			}

			that->_SetError(true, std::move(finalError));

			if (that->_isRefreshPending) {
				that->_isRefreshPending = false;
				that->RequestRefresh(true);
			}
			co_return;
		}

		SoftwareBitmap previewBitmap(
			BitmapPixelFormat::Bgra8,
			(int32_t)previewResult.width,
			(int32_t)previewResult.height,
			BitmapAlphaMode::Ignore
		);
		{
			BitmapBuffer buffer = previewBitmap.LockBuffer(BitmapBufferAccessMode::Write);
			IMemoryBufferReference bufferRef = buffer.CreateReference();
			uint8_t* dst = bufferRef.data();
			const BitmapPlaneDescription planeDesc = buffer.GetPlaneDescription(0);
			const size_t copyWidth = (size_t)previewResult.width * 4;

			for (uint32_t y = 0; y < previewResult.height; ++y) {
				const uint8_t* srcRow = previewResult.pixels.data() + (size_t)y * previewResult.rowPitch;
				uint8_t* dstRow = dst + planeDesc.StartIndex + (size_t)y * planeDesc.Stride;
				std::memcpy(dstRow, srcRow, copyWidth);
			}
		}

		SoftwareBitmapSource imageSource;
		co_await imageSource.SetBitmapAsync(previewBitmap);
		co_await *ui;

		that = weakThis.lock();
		if (!that || that->_requestId != requestId) {
			co_return;
		}

		that->_SetImage(std::move(imageSource));

		if (that->_isRefreshPending) {
			that->_isRefreshPending = false;
			that->RequestRefresh(true);
		}
	} catch (const hresult_error& e) {
		Logger::Get().ComError("预览刷新失败", e.code());
		hasException = true;
		exceptionMessage = e.message().empty() ?
			L"Preview refresh failed." : e.message();
	} catch (...) {
		Logger::Get().Error("预览刷新失败: 未知异常");
		hasException = true;
		exceptionMessage = L"Preview refresh failed.";
	}

	if (!hasException) {
		co_return;
	}

	if (ui) {
		co_await *ui;
	} else {
		co_await App::Get().Dispatcher();
	}

	auto that = weakThis.lock();
	if (!that || that->_requestId != requestId) {
		co_return;
	}

	that->_SetRendering(false);
	that->_SetError(true, std::move(exceptionMessage));
}

}
