#pragma once
#include "ScalingModesPage.g.h"
#include "ScalingModeItem.h"
#include "ScalingModesViewModel.h"

namespace winrt::Magpie::implementation {

struct ScalingModesPage : ScalingModesPageT<ScalingModesPage> {
	ScalingModesPage();
	void OnNavigatedFrom(Navigation::NavigationEventArgs const& args);

	winrt::Magpie::ScalingModesViewModel ViewModel() const noexcept {
		return *_viewModel;
	}

	void ComboBox_DropDownOpened(IInspectable const& sender, IInspectable const&);

	void NumberBox_Loaded(IInspectable const& sender, RoutedEventArgs const&);

	void EffectSettingsCard_Loaded(IInspectable const& sender, RoutedEventArgs const&);

	void PreviewContainer_SizeChanged(
		IInspectable const& sender,
		Windows::UI::Xaml::SizeChangedEventArgs const& args
	);

	void PreviewScrollViewer_SizeChanged(
		IInspectable const& sender,
		Windows::UI::Xaml::SizeChangedEventArgs const&
	);

	void PreviewScrollViewer_PointerPressed(
		IInspectable const& sender,
		Windows::UI::Xaml::Input::PointerRoutedEventArgs const& args
	);

	void PreviewScrollViewer_PointerMoved(
		IInspectable const& sender,
		Windows::UI::Xaml::Input::PointerRoutedEventArgs const& args
	);

	void PreviewScrollViewer_PointerReleased(
		IInspectable const& sender,
		Windows::UI::Xaml::Input::PointerRoutedEventArgs const& args
	);

	void PreviewScrollViewer_PointerCanceled(
		IInspectable const& sender,
		Windows::UI::Xaml::Input::PointerRoutedEventArgs const& args
	);

	void PreviewScrollViewer_PointerCaptureLost(
		IInspectable const& sender,
		Windows::UI::Xaml::Input::PointerRoutedEventArgs const& args
	);

	void PreviewScrollViewer_PointerWheelChanged(
		IInspectable const& sender,
		Windows::UI::Xaml::Input::PointerRoutedEventArgs const& args
	);

	void ZoomResetButton_Click(IInspectable const& sender, RoutedEventArgs const&);

	void AddEffectButton_Click(IInspectable const& sender, RoutedEventArgs const&);

	void NewScalingModeFlyout_Opening(IInspectable const&, IInspectable const&);

	void NewScalingModeNameTextBox_KeyDown(IInspectable const&, Input::KeyRoutedEventArgs const& args);

	void NewScalingModeConfirmButton_Click(IInspectable const& sender, RoutedEventArgs const&);

	void ScalingModeMoreOptionsButton_Click(IInspectable const& sender, RoutedEventArgs const&);

	void RemoveScalingModeMenuItem_Click(IInspectable const& sender, RoutedEventArgs const&);
private:
	void _BuildEffectMenu() noexcept;

	void _AddEffectMenuFlyoutItem_Click(IInspectable const& sender, RoutedEventArgs const&);

	IInspectable _moreOptionsButton{ nullptr };
	Windows::UI::Xaml::Controls::ScrollViewer _activePreviewDragScrollViewer{ nullptr };
	bool _isPreviewDragging = false;
	uint32_t _previewDragPointerId = 0;
	Windows::Foundation::Point _previewLastPointerPos{};

	MenuFlyout _addEffectMenuFlyout;
	com_ptr<ScalingModesViewModel> _viewModel = make_self<ScalingModesViewModel>();
	ScalingModeItem* _curScalingMode = nullptr;
};

}

BASIC_FACTORY(ScalingModesPage)
