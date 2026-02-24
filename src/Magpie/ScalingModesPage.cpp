#include "pch.h"
#include "ScalingModesPage.h"
#if __has_include("ScalingModesPage.g.cpp")
#include "ScalingModesPage.g.cpp"
#endif
#include "ControlHelper.h"
#include "EffectsService.h"
#include <parallel_hashmap/phmap.h>
#include <winrt/Windows.UI.Input.h>

using namespace ::Magpie;
using namespace winrt;
using namespace Windows::UI::Xaml::Controls::Primitives;
using namespace Windows::UI::Xaml::Input;

namespace winrt::Magpie::implementation {

namespace Controls = Windows::UI::Xaml::Controls;
namespace Media = Windows::UI::Xaml::Media;

static constexpr float WHEEL_ZOOM_STEP = 1.12f;

static double ClampOffset(double value, double maxOffset) noexcept {
	return std::min(std::max(value, 0.0), std::max(maxOffset, 0.0));
}

static Controls::ScrollViewer FindPreviewScrollViewer(DependencyObject const& element) noexcept {
	DependencyObject current = Media::VisualTreeHelper::GetParent(element);
	while (current) {
		auto panel = current.try_as<Controls::Panel>();
		if (panel) {
			for (UIElement const& child : panel.Children()) {
				auto sv = child.try_as<Controls::ScrollViewer>();
				if (sv) {
					return sv;
				}
			}
		}

		current = Media::VisualTreeHelper::GetParent(current);
	}

	return nullptr;
}

ScalingModesPage::ScalingModesPage() {
	_BuildEffectMenu();
}

void ScalingModesPage::OnNavigatedFrom(Navigation::NavigationEventArgs const&) {
	if (_activePreviewDragScrollViewer) {
		_activePreviewDragScrollViewer.ReleasePointerCaptures();
	}
	_activePreviewDragScrollViewer = nullptr;
	_isPreviewDragging = false;
	_previewDragPointerId = 0;

	if (_viewModel) {
		_viewModel->StopPreviews();
	}
}

void ScalingModesPage::ComboBox_DropDownOpened(IInspectable const& sender, IInspectable const&) {
	ControlHelper::ComboBox_DropDownOpened(sender);
}

void ScalingModesPage::NumberBox_Loaded(IInspectable const& sender, RoutedEventArgs const&) {
	ControlHelper::NumberBox_Loaded(sender);
}

void ScalingModesPage::EffectSettingsCard_Loaded(IInspectable const& sender, RoutedEventArgs const&) {
	XamlHelper::UpdateThemeOfTooltips(sender.try_as<DependencyObject>(), ActualTheme());
}

void ScalingModesPage::PreviewContainer_SizeChanged(
	IInspectable const& sender,
	Windows::UI::Xaml::SizeChangedEventArgs const& args
) {
	FrameworkElement element = sender.try_as<FrameworkElement>();
	if (!element) {
		return;
	}

	winrt::Magpie::ScalingModeItem item = element.DataContext().try_as<winrt::Magpie::ScalingModeItem>();
	if (!item) {
		return;
	}

	get_self<ScalingModeItem>(item)->PreviewContainerSizeChanged(sender, args);
}

void ScalingModesPage::PreviewScrollViewer_SizeChanged(
	IInspectable const& sender,
	Windows::UI::Xaml::SizeChangedEventArgs const&
) {
	auto sv = sender.try_as<Controls::ScrollViewer>();
	if (!sv) {
		return;
	}

	auto image = sv.Content().try_as<Controls::Image>();
	if (!image) {
		return;
	}

	const double vw = sv.ViewportWidth();
	const double vh = sv.ViewportHeight();
	if (vw > 0 && vh > 0) {
		image.Width(vw);
		image.Height(vh);
	}
}

void ScalingModesPage::PreviewScrollViewer_PointerPressed(
	IInspectable const& sender,
	PointerRoutedEventArgs const& args
) {
	auto sv = sender.try_as<Controls::ScrollViewer>();
	if (!sv) {
		return;
	}

	auto point = args.GetCurrentPoint(sv);
	if (!point.Properties().IsLeftButtonPressed()) {
		return;
	}

	_activePreviewDragScrollViewer = sv;
	_isPreviewDragging = true;
	_previewDragPointerId = point.PointerId();
	_previewLastPointerPos = point.Position();
	sv.CapturePointer(args.Pointer());
	args.Handled(true);
}

void ScalingModesPage::PreviewScrollViewer_PointerMoved(
	IInspectable const& sender,
	PointerRoutedEventArgs const& args
) {
	auto sv = sender.try_as<Controls::ScrollViewer>();
	if (!sv || !_isPreviewDragging || sv != _activePreviewDragScrollViewer) {
		return;
	}

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

void ScalingModesPage::PreviewScrollViewer_PointerReleased(
	IInspectable const& sender,
	PointerRoutedEventArgs const& args
) {
	auto sv = sender.try_as<Controls::ScrollViewer>();
	if (!sv || !_isPreviewDragging || sv != _activePreviewDragScrollViewer) {
		return;
	}

	auto point = args.GetCurrentPoint(sv);
	if (point.PointerId() != _previewDragPointerId) {
		return;
	}

	_activePreviewDragScrollViewer = nullptr;
	_isPreviewDragging = false;
	_previewDragPointerId = 0;
	args.Handled(true);
}

void ScalingModesPage::PreviewScrollViewer_PointerCanceled(
	IInspectable const& sender,
	PointerRoutedEventArgs const& args
) {
	auto sv = sender.try_as<Controls::ScrollViewer>();
	if (!sv || !_isPreviewDragging || sv != _activePreviewDragScrollViewer) {
		return;
	}

	auto point = args.GetCurrentPoint(sv);
	if (point.PointerId() != _previewDragPointerId) {
		return;
	}

	_activePreviewDragScrollViewer = nullptr;
	_isPreviewDragging = false;
	_previewDragPointerId = 0;
	args.Handled(true);
}

void ScalingModesPage::PreviewScrollViewer_PointerCaptureLost(
	IInspectable const& sender,
	PointerRoutedEventArgs const&
) {
	auto sv = sender.try_as<Controls::ScrollViewer>();
	if (!sv || !_isPreviewDragging || sv != _activePreviewDragScrollViewer) {
		return;
	}

	_activePreviewDragScrollViewer = nullptr;
	_isPreviewDragging = false;
	_previewDragPointerId = 0;
}

void ScalingModesPage::PreviewScrollViewer_PointerWheelChanged(
	IInspectable const& sender,
	PointerRoutedEventArgs const& args
) {
	auto sv = sender.try_as<Controls::ScrollViewer>();
	if (!sv) {
		return;
	}

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

void ScalingModesPage::ZoomResetButton_Click(IInspectable const& sender, RoutedEventArgs const&) {
	auto sv = FindPreviewScrollViewer(sender.try_as<DependencyObject>());
	if (!sv) {
		return;
	}

	sv.ChangeView(0.0, 0.0, 1.0f);
}

void ScalingModesPage::AddEffectButton_Click(IInspectable const& sender, RoutedEventArgs const&) {
	Button btn = sender.try_as<Button>();
	_curScalingMode = get_self<ScalingModeItem>(btn.Tag().try_as<winrt::Magpie::ScalingModeItem>());
	_addEffectMenuFlyout.ShowAt(btn);
}

void ScalingModesPage::NewScalingModeFlyout_Opening(IInspectable const&, IInspectable const&) {
	_viewModel->PrepareForAdd();
}

void ScalingModesPage::NewScalingModeNameTextBox_KeyDown(IInspectable const&, KeyRoutedEventArgs const& args) {
	if (args.Key() == VirtualKey::Enter) {
		if (_viewModel->IsAddButtonEnabled()) {
			NewScalingModeConfirmButton_Click(nullptr, nullptr);
		}
	}
}

void ScalingModesPage::NewScalingModeConfirmButton_Click(IInspectable const&, RoutedEventArgs const&) {
	NewScalingModeFlyout().Hide();
	_viewModel->AddScalingMode();
}

void ScalingModesPage::ScalingModeMoreOptionsButton_Click(IInspectable const& sender, RoutedEventArgs const&) {
	_moreOptionsButton = sender;
}

void ScalingModesPage::RemoveScalingModeMenuItem_Click(IInspectable const& sender, RoutedEventArgs const&) {
	MenuFlyoutItem menuItem = sender.try_as<MenuFlyoutItem>();
	ScalingModeItem* scalingModeItem = get_self<ScalingModeItem>(menuItem.Tag().try_as<winrt::Magpie::ScalingModeItem>());
	if (scalingModeItem->IsInUse()) {
		// 如果有缩放配置正在使用此缩放模式则弹出确认弹窗
		FlyoutBase::GetAttachedFlyout(menuItem)
			.ShowAt(_moreOptionsButton.try_as<FrameworkElement>());
	} else {
		scalingModeItem->Remove();
	}
}

void ScalingModesPage::_BuildEffectMenu() noexcept {
	std::vector<MenuFlyoutItemBase> rootItems;

	phmap::flat_hash_map<std::wstring_view, MenuFlyoutSubItem> folders;
	folders.reserve(13);
	for (const auto& effect : EffectsService::Get().Effects()) {
		std::wstring_view name(effect.name);

		MenuFlyoutItem item;
		item.Tag(box_value(effect.name));
		item.Click({ this, &ScalingModesPage::_AddEffectMenuFlyoutItem_Click });

		size_t delimPos = name.find_last_of(L'\\');
		if (delimPos == std::wstring::npos) {
			item.Text(name);
			rootItems.emplace_back(std::move(item));
			continue;
		}

		item.Text(name.substr(delimPos + 1));

		std::wstring_view dir = name.substr(0, delimPos);
		auto it = folders.find(dir);
		if (it != folders.end()) {
			it->second.Items().Append(item);
		} else {
			MenuFlyoutSubItem folder;
			folder.Text(hstring(dir));
			folder.Items().Append(item);

			rootItems.push_back(folder);
			folders.emplace(dir, folder);
		}
	}

	std::sort(rootItems.begin(), rootItems.end(), [](MenuFlyoutItemBase const& l, MenuFlyoutItemBase const& r) {
		bool isLSubMenu = get_class_name(l) == name_of<MenuFlyoutSubItem>();
		bool isRSubMenu = get_class_name(r) == name_of<MenuFlyoutSubItem>();

		if (isLSubMenu != isRSubMenu) {
			return isLSubMenu;
		}

		if (isLSubMenu) {
			return l.try_as<MenuFlyoutSubItem>().Text() < r.try_as<MenuFlyoutSubItem>().Text();
		} else {
			return l.try_as<MenuFlyoutItem>().Text() < r.try_as<MenuFlyoutItem>().Text();
		}
	});

	// 排序文件夹中的项目
	for (MenuFlyoutItemBase& item : rootItems) {
		MenuFlyoutSubItem folder = item.try_as<MenuFlyoutSubItem>();
		if (!folder) {
			break;
		}

		IVector<MenuFlyoutItemBase> items = folder.Items();
		// 读取到 std::vector 中以提高排序性能
		std::vector<MenuFlyoutItemBase> itemsVec(items.Size(), nullptr);
		items.GetMany(0, itemsVec);
		std::sort(itemsVec.begin(), itemsVec.end(), [](const MenuFlyoutItemBase& l, const MenuFlyoutItemBase& r) {
			hstring lEffectName = unbox_value<hstring>(l.try_as<MenuFlyoutItem>().Tag());
			hstring rEffectName = unbox_value<hstring>(r.try_as<MenuFlyoutItem>().Tag());

			const EffectInfo* lEffectInfo = EffectsService::Get().GetEffect(lEffectName);
			const EffectInfo* rEffectInfo = EffectsService::Get().GetEffect(rEffectName);

			return lEffectInfo->sortName < rEffectInfo->sortName;
		});
		items.ReplaceAll(itemsVec);
	}

	for (MenuFlyoutItemBase& item : rootItems) {
		_addEffectMenuFlyout.Items().Append(std::move(item));
	}
}

void ScalingModesPage::_AddEffectMenuFlyoutItem_Click(IInspectable const& sender, RoutedEventArgs const&) {
	hstring effectName = unbox_value<hstring>(sender.try_as<MenuFlyoutItem>().Tag());
	_curScalingMode->AddEffect(effectName);
}

}
