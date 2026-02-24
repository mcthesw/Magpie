#pragma once
#include "ProfilePage.g.h"
#include "ProfileViewModel.h"

namespace winrt::Magpie::implementation {

struct ProfilePage : ProfilePageT<ProfilePage> {
	void OnNavigatedTo(Navigation::NavigationEventArgs const& args);
	void OnNavigatedFrom(Navigation::NavigationEventArgs const& args);

	winrt::Magpie::ProfileViewModel ViewModel() const noexcept {
		return *_viewModel;
	}

	void ComboBox_DropDownOpened(IInspectable const& sender, IInspectable const&);

	void NumberBox_Loaded(IInspectable const& sender, RoutedEventArgs const&);

	void PreviewContainer_SizeChanged(
		IInspectable const& sender,
		Windows::UI::Xaml::SizeChangedEventArgs const& args
	);

	void PreviewScrollViewer_SizeChanged(
		IInspectable const&, Windows::UI::Xaml::SizeChangedEventArgs const&
	);

	void PreviewScrollViewer_PointerPressed(
		IInspectable const&, Windows::UI::Xaml::Input::PointerRoutedEventArgs const& args
	);

	void PreviewScrollViewer_PointerMoved(
		IInspectable const&, Windows::UI::Xaml::Input::PointerRoutedEventArgs const& args
	);

	void PreviewScrollViewer_PointerReleased(
		IInspectable const&, Windows::UI::Xaml::Input::PointerRoutedEventArgs const& args
	);

	void PreviewScrollViewer_PointerCanceled(
		IInspectable const&, Windows::UI::Xaml::Input::PointerRoutedEventArgs const& args
	);

	void PreviewScrollViewer_PointerCaptureLost(
		IInspectable const&, Windows::UI::Xaml::Input::PointerRoutedEventArgs const& args
	);

	void PreviewScrollViewer_PointerWheelChanged(
		IInspectable const&, Windows::UI::Xaml::Input::PointerRoutedEventArgs const& args
	);

	void ZoomResetButton_Click(IInspectable const&, RoutedEventArgs const&);

	void InitialWindowedScaleFactorComboBox_SelectionChanged(IInspectable const&, SelectionChangedEventArgs const&);

	void CursorScalingComboBox_SelectionChanged(IInspectable const&, SelectionChangedEventArgs const&);

	void RenameMenuItem_Click(IInspectable const&, RoutedEventArgs const&);

	void RenameFlyout_Opening(IInspectable const&, IInspectable const&);

	void RenameConfirmButton_Click(IInspectable const&, RoutedEventArgs const&);

	void RenameTextBox_KeyDown(IInspectable const&, Input::KeyRoutedEventArgs const& args);

	void ReorderMenuItem_Click(IInspectable const&, RoutedEventArgs const&);

	void DeleteMenuItem_Click(IInspectable const&, RoutedEventArgs const&);

	void DeleteButton_Click(IInspectable const&, RoutedEventArgs const&);

	void LaunchParametersTextBox_KeyDown(IInspectable const&, Input::KeyRoutedEventArgs const& args);

private:
	com_ptr<ProfileViewModel> _viewModel;
	bool _isPreviewDragging = false;
	uint32_t _previewDragPointerId = 0;
	Windows::Foundation::Point _previewLastPointerPos{};
};

}

BASIC_FACTORY(ProfilePage)
