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
        void On_Common(PLTWinRt::ErrorCode ec, Platform::String^ device_id);
        void On_GetPositionInfo(PLTWinRt::ErrorCode ec, Platform::String^ device_id, PLTWinRt::DMR_PositionInfo^ pi);
        void On_GetVolume(PLTWinRt::ErrorCode ec, Platform::String^ device_id, unsigned int vol);
        void On_GetCap(PLTWinRt::ErrorCode ec, Platform::String^ device_id, Windows::Foundation::Collections::IVector<Platform::String^>^ pm, Windows::Foundation::Collections::IVector<Platform::String^>^ cm, Windows::Foundation::Collections::IVector<Platform::String^>^ cqm);
        void On_GetTrans(PLTWinRt::ErrorCode ec, Platform::String^ device_id, PLTWinRt::DMR_TransportInfo^ trans);
        void On_GetMute(PLTWinRt::ErrorCode ec, Platform::String^ device_id,bool is_mute);

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
        void OnGetPosClicked(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
        void OnGetVolumeClicked(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
        void OnGetCapClicked(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
        void OnGetTransClicked(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
        void OnGetMuteClicked(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
    };
}
