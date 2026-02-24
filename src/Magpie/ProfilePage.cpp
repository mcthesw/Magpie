#include "pch.h"
#include "ProfilePage.h"
#if __has_include("ProfilePage.g.cpp")
#include "ProfilePage.g.cpp"
#endif
#include "App.h"
#include "ControlHelper.h"
#include "Profile.h"
#include <winrt/Windows.UI.Input.h>

using namespace ::Magpie;
using namespace winrt;
using namespace Windows::UI::Xaml::Controls::Primitives;
using namespace Windows::UI::Xaml::Input;

namespace winrt::Magpie::implementation {

static constexpr float WHEEL_ZOOM_STEP = 1.12f;

static double ClampOffset(double value, double maxOffset) noexcept {
	return std::min(std::max(value, 0.0), std::max(maxOffset, 0.0));
}

void ProfilePage::OnNavigatedTo(Navigation::NavigationEventArgs const& args) {
	int profileIdx = args.Parameter().try_as<int>().value();
	_viewModel = make_self<ProfileViewModel>(profileIdx);
}

void ProfilePage::OnNavigatedFrom(Navigation::NavigationEventArgs const&) {
	_isPreviewDragging = false;
	_previewDragPointerId = 0;

	if (_viewModel) {
		_viewModel->StopPreview();
		_viewModel = nullptr;
	}
}

void ProfilePage::ComboBox_DropDownOpened(IInspectable const& sender, IInspectable const&) {
	ControlHelper::ComboBox_DropDownOpened(sender);
}

void ProfilePage::NumberBox_Loaded(IInspectable const& sender, RoutedEventArgs const&) {
	ControlHelper::NumberBox_Loaded(sender);
}

void ProfilePage::PreviewContainer_SizeChanged(
	IInspectable const& sender,
	Windows::UI::Xaml::SizeChangedEventArgs const& args
) {
	_viewModel->PreviewContainerSizeChanged(sender, args);
}

void ProfilePage::PreviewScrollViewer_SizeChanged(
	IInspectable const&,
	Windows::UI::Xaml::SizeChangedEventArgs const&
) {
	const auto sv = PreviewScrollViewer();
	const double vw = sv.ViewportWidth();
	const double vh = sv.ViewportHeight();
	if (vw > 0 && vh > 0) {
		PreviewImageElement().Width(vw);
		PreviewImageElement().Height(vh);
	}
}

void ProfilePage::PreviewScrollViewer_PointerPressed(
	IInspectable const&,
	PointerRoutedEventArgs const& args
) {
	auto sv = PreviewScrollViewer();
	auto point = args.GetCurrentPoint(sv);
	if (!point.Properties().IsLeftButtonPressed()) {
		return;
	}

	_isPreviewDragging = true;
	_previewDragPointerId = point.PointerId();
	_previewLastPointerPos = point.Position();
	sv.CapturePointer(args.Pointer());
	args.Handled(true);
}

void ProfilePage::PreviewScrollViewer_PointerMoved(
	IInspectable const&,
	PointerRoutedEventArgs const& args
) {
	if (!_isPreviewDragging) {
		return;
	}

	auto sv = PreviewScrollViewer();
	auto point = args.GetCurrentPoint(sv);
	if (point.PointerId() != _previewDragPointerId) {
		return;
	}

	Windows::Foundation::Point curPos = point.Position();
	double dx = curPos.X - _previewLastPointerPos.X;
	double dy = curPos.Y - _previewLastPointerPos.Y;
	double newX = ClampOffset(sv.HorizontalOffset() - dx, sv.ScrollableWidth());
	double newY = ClampOffset(sv.VerticalOffset() - dy, sv.ScrollableHeight());

	sv.ChangeView(
		newX,
		newY,
		nullptr,
		true  // disable animation for immediate 1:1 tracking
	);

	_previewLastPointerPos = curPos;
	args.Handled(true);
}

void ProfilePage::PreviewScrollViewer_PointerReleased(
	IInspectable const&,
	PointerRoutedEventArgs const& args
) {
	if (!_isPreviewDragging) {
		return;
	}

	auto point = args.GetCurrentPoint(PreviewScrollViewer());
	if (point.PointerId() != _previewDragPointerId) {
		return;
	}

	_isPreviewDragging = false;
	_previewDragPointerId = 0;
	args.Handled(true);
}

void ProfilePage::PreviewScrollViewer_PointerCanceled(
	IInspectable const&,
	PointerRoutedEventArgs const& args
) {
	if (!_isPreviewDragging) {
		return;
	}

	auto point = args.GetCurrentPoint(PreviewScrollViewer());
	if (point.PointerId() != _previewDragPointerId) {
		return;
	}

	_isPreviewDragging = false;
	_previewDragPointerId = 0;
	args.Handled(true);
}

void ProfilePage::PreviewScrollViewer_PointerCaptureLost(
	IInspectable const&,
	PointerRoutedEventArgs const&
) {
	_isPreviewDragging = false;
	_previewDragPointerId = 0;
}

void ProfilePage::PreviewScrollViewer_PointerWheelChanged(
	IInspectable const&,
	PointerRoutedEventArgs const& args
) {
	auto sv = PreviewScrollViewer();
	auto point = args.GetCurrentPoint(sv);
	int32_t delta = point.Properties().MouseWheelDelta();
	if (!delta) {
		return;
	}

	const auto pos = point.Position();
	double z = sv.ZoomFactor();
	float newZoom = static_cast<float>(z) * (delta > 0 ? WHEEL_ZOOM_STEP : 1.0f / WHEEL_ZOOM_STEP);
	newZoom = std::min(std::max(newZoom, sv.MinZoomFactor()), sv.MaxZoomFactor());
	double z2 = newZoom;

	double ox = sv.HorizontalOffset();
	double oy = sv.VerticalOffset();
	double cx = (ox + pos.X) / z;
	double cy = (oy + pos.Y) / z;

	double ox2 = cx * z2 - pos.X;
	double oy2 = cy * z2 - pos.Y;
	ox2 = ClampOffset(ox2, sv.ScrollableWidth());
	oy2 = ClampOffset(oy2, sv.ScrollableHeight());

	sv.ChangeView(ox2, oy2, newZoom, true);
	args.Handled(true);
}

void ProfilePage::ZoomResetButton_Click(IInspectable const&, RoutedEventArgs const&) {
	// Reset zoom to 1 and scroll to origin so image is fully visible
	PreviewScrollViewer().ChangeView(0.0, 0.0, 1.0f);
}

void ProfilePage::InitialWindowedScaleFactorComboBox_SelectionChanged(IInspectable const&, SelectionChangedEventArgs const&) {
	if ((InitialWindowedScaleFactor)_viewModel->InitialWindowedScaleFactor() == InitialWindowedScaleFactor::Custom) {
		InitialWindowedScaleFactorComboBox().MinWidth(0);
		CustomInitialWindowedScaleFactorNumberBox().Visibility(Visibility::Visible);
		CustomInitialWindowedScaleFactorLabel().Visibility(Visibility::Visible);
	} else {
		const double minWidth = App::Get().Resources()
			.Lookup(box_value(L"SettingsCardContentMinWidth"))
			.try_as<double>().value();
		InitialWindowedScaleFactorComboBox().MinWidth(minWidth);
		CustomInitialWindowedScaleFactorNumberBox().Visibility(Visibility::Collapsed);
		CustomInitialWindowedScaleFactorLabel().Visibility(Visibility::Collapsed);
	}
}

void ProfilePage::CursorScalingComboBox_SelectionChanged(IInspectable const&, SelectionChangedEventArgs const&) {
	if ((CursorScaling)_viewModel->CursorScaling() == CursorScaling::Custom) {
		CursorScalingComboBox().MinWidth(0);
		CustomCursorScalingNumberBox().Visibility(Visibility::Visible);
		CustomCursorScalingLabel().Visibility(Visibility::Visible);
	} else {
		const double minWidth = App::Get().Resources()
			.Lookup(box_value(L"SettingsCardContentMinWidth"))
			.try_as<double>().value();
		CursorScalingComboBox().MinWidth(minWidth);
		CustomCursorScalingNumberBox().Visibility(Visibility::Collapsed);
		CustomCursorScalingLabel().Visibility(Visibility::Collapsed);
	}
}

void ProfilePage::RenameMenuItem_Click(IInspectable const&, RoutedEventArgs const&) {
	RenameFlyout().ShowAt(MoreOptionsButton());
}

void ProfilePage::RenameFlyout_Opening(IInspectable const&, IInspectable const&) {
	TextBox tb = RenameTextBox();
	hstring name = _viewModel->Name();
	tb.Text(name);
	tb.SelectionStart(name.size());
}

void ProfilePage::RenameConfirmButton_Click(IInspectable const&, RoutedEventArgs const&) {
	RenameFlyout().Hide();
	_viewModel->Rename();
}

void ProfilePage::RenameTextBox_KeyDown(IInspectable const&, Input::KeyRoutedEventArgs const& args) {
	if (args.Key() == VirtualKey::Enter && _viewModel->IsRenameConfirmButtonEnabled()) {
		RenameConfirmButton_Click(nullptr, nullptr);
	}
}

void ProfilePage::ReorderMenuItem_Click(IInspectable const&, RoutedEventArgs const&) {
	ReorderFlyout().ShowAt(MoreOptionsButton());
}

void ProfilePage::DeleteMenuItem_Click(IInspectable const&, RoutedEventArgs const&) {
	DeleteFlyout().ShowAt(MoreOptionsButton());
}

void ProfilePage::DeleteButton_Click(IInspectable const&, RoutedEventArgs const&) {
	DeleteFlyout().Hide();
	_viewModel->Delete();
}

void ProfilePage::LaunchParametersTextBox_KeyDown(IInspectable const&, Input::KeyRoutedEventArgs const& args) {
	if (args.Key() == VirtualKey::Enter) {
		Focus(FocusState::Pointer);
	}
}

}
