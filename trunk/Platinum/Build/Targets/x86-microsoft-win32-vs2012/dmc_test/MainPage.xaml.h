//
// MainPage.xaml.h
// Declaration of the MainPage class.
//

#pragma once

#include "MainPage.g.h"
#include "DeviceData.h"

namespace dmc_test
{
	/// <summary>
	/// An empty page that can be used on its own or navigated to within a Frame.
	/// </summary>
	public ref class MainPage sealed
	{
	public:
		MainPage();

    protected:
        void OnPlayStateChange(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
        void OnBufferingProgressChanged(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
        void OnSeekCompleted(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
        void OnVolumeChanged(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);

	protected:
		virtual void OnNavigatedTo(Windows::UI::Xaml::Navigation::NavigationEventArgs^ e) override;

    private:
        void On_DeviceAdd(Platform::String^ device_id, Platform::String^ divice_name, bool is_dmr);
        void On_DeviceDel(Platform::String^ device_id, Platform::String^ divice_name, bool is_dmr);

    private:
        void StartDmcBtnClicked(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
        void PlayMedia(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
        void StopMedia(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
        void PauseMedia(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
        void ProcessBar_ValueChanged(Platform::Object^ sender, Windows::UI::Xaml::Controls::Primitives::RangeBaseValueChangedEventArgs^ e);
        void Slider_ValueChanged_1(Platform::Object^ sender, Windows::UI::Xaml::Controls::Primitives::RangeBaseValueChangedEventArgs^ e);
        void DmrItemListView_SelectionChanged(Platform::Object^ sender, Windows::UI::Xaml::Controls::SelectionChangedEventArgs^ e);

        void PageLoadedHandler(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
    private:
        Platform::String^           m_current_dmr_;
        Platform::String^           m_current_dms_;
        Devices^                    m_dmrs_;
        Devices^                    m_dmss_;
        PLTWinRt::MediaController^  m_controller_;
    };
}
