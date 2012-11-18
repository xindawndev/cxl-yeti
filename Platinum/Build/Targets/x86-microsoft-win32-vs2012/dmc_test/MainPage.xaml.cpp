//
// MainPage.xaml.cpp
// Implementation of the MainPage class.
//

#include "pch.h"
#include "MainPage.xaml.h"

using namespace dmc_test;

using namespace Platform;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Core;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Controls::Primitives;
using namespace Windows::UI::Xaml::Data;
using namespace Windows::UI::Xaml::Input;
using namespace Windows::UI::Xaml::Media;
using namespace Windows::UI::Xaml::Navigation;
using namespace PLTWinRt;

// The Blank Page item template is documented at http://go.microsoft.com/fwlink/?LinkId=234238

MainPage::MainPage()
{
	InitializeComponent();
    m_dmrs_ = ref new Devices();
    m_dmss_ = ref new Devices();

    MediaPlayer->CurrentStateChanged += ref new Windows::UI::Xaml::RoutedEventHandler(this, &MainPage::OnPlayStateChange);
    MediaPlayer->BufferingProgressChanged += ref new Windows::UI::Xaml::RoutedEventHandler(this, &MainPage::OnBufferingProgressChanged);
    MediaPlayer->SeekCompleted += ref new Windows::UI::Xaml::RoutedEventHandler(this, &MainPage::OnSeekCompleted);
    MediaPlayer->VolumeChanged += ref new Windows::UI::Xaml::RoutedEventHandler(this, &MainPage::OnVolumeChanged);

    m_controller_ = ref new MediaController();
    m_controller_->onDeviceAdd += ref new OnDeviceAdd(this, &MainPage::On_DeviceAdd);
    m_controller_->onDeviceDel += ref new OnDeviceDel(this, &MainPage::On_DeviceDel);
}

void MainPage::OnPlayStateChange(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    Platform::String^ msg;
    switch (MediaPlayer->CurrentState) {
    case Windows::UI::Xaml::Media::MediaElementState::Opening:
        msg = "正在打开~";
        break;
    case Windows::UI::Xaml::Media::MediaElementState::Buffering:
        msg = "正在缓冲~";
        break;
    case Windows::UI::Xaml::Media::MediaElementState::Playing:
        msg = "正在播放~";
        break;
    case Windows::UI::Xaml::Media::MediaElementState::Paused:
        msg = "已暂停~";
        break;
    case Windows::UI::Xaml::Media::MediaElementState::Stopped:
        msg = "已停止~";
        break;
    case Windows::UI::Xaml::Media::MediaElementState::Closed:
        msg = "已关闭~";
        break;
    default:
        msg = "未知状态！";
        break;
    }
    ShowInfo->Text = msg;
}

void MainPage::OnBufferingProgressChanged(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    ShowInfo->Text = e->ToString();
}

void MainPage::OnSeekCompleted(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    ShowInfo->Text = "Seek Completed: " + MediaPlayer->Position.Duration.ToString();
}

void MainPage::OnVolumeChanged(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    ShowInfo->Text = "OnVolumeChanged: " + MediaPlayer->Volume.ToString();
}

void MainPage::On_DeviceAdd(Platform::String^ device_id, Platform::String^ divice_name, bool is_dmr)
{
    DeviceInfo^ di = ref new DeviceInfo();
    di->DeviceId = device_id;
    di->DeviceName = divice_name;
    if (is_dmr) {
        this->Dispatcher->RunAsync(CoreDispatcherPriority::Normal, ref new DispatchedHandler([this, device_id, divice_name] () {
            ShowInfo->Text += "Add DMR: " + device_id + " Name: " + divice_name + "\r\n";
        }));
        m_current_dmr_ = device_id;
        m_dmrs_->Items->Append(di);
        this->DataContext = m_dmrs_;
    } else {
        m_dmss_->Items->Append(di);
    }
}

void MainPage::On_DeviceDel(Platform::String^ device_id, Platform::String^ divice_name, bool is_dmr)
{
    if (is_dmr) {
        this->Dispatcher->RunAsync(CoreDispatcherPriority::Normal, ref new DispatchedHandler([this, device_id, divice_name] () {
            ShowInfo->Text += "Remove DMR: " + device_id + " Name: " + divice_name + "\r\n";
        }));

        for (int i = 0; i < (int)m_dmss_->Items->Size; ++i) {

            if (m_dmrs_->Items->GetAt(i)->DeviceId == device_id && 
                m_dmrs_->Items->GetAt(i)->DeviceName == divice_name)
                m_dmrs_->Items->RemoveAt(i);
        }
        this->DataContext = m_dmrs_;
    } else {
        for (int i = 0; i < (int)m_dmss_->Items->Size; ++i) {
            if (m_dmss_->Items->GetAt(i)->DeviceId == device_id && 
                m_dmss_->Items->GetAt(i)->DeviceName == divice_name)
                m_dmss_->Items->RemoveAt(i);
        }
    }
}

/// <summary>
/// Invoked when this page is about to be displayed in a Frame.
/// </summary>
/// <param name="e">Event data that describes how this page was reached.  The Parameter
/// property is typically used to configure the page.</param>
void MainPage::OnNavigatedTo(NavigationEventArgs^ e)
{
	(void) e;	// Unused parameter
}

void MainPage::StartDmcBtnClicked(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    Platform::String^ play_str = MediaUrl->Text;
    if (play_str->IsEmpty()) {
        ShowInfo->Text = "Url为空，失败！";
        return;
    }

    m_controller_->DmrSetAVTransportURI(m_current_dmr_, play_str, nullptr);
    Windows::Foundation::Uri^ url = ref new Windows::Foundation::Uri(play_str);
    MediaPlayer->Source = url;
    VolumeBar->Value = MediaPlayer->Volume;
}

void MainPage::PlayMedia(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    if (MediaPlayer->CurrentState == Windows::UI::Xaml::Media::MediaElementState::Stopped
        || MediaPlayer->CurrentState == Windows::UI::Xaml::Media::MediaElementState::Paused) {
            MediaPlayer->Play();
    }
    m_controller_->DmrPlay(m_current_dmr_);
}

void MainPage::StopMedia(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    if (MediaPlayer->CurrentState == Windows::UI::Xaml::Media::MediaElementState::Playing
        || MediaPlayer->CurrentState == Windows::UI::Xaml::Media::MediaElementState::Paused) {
            MediaPlayer->Stop();
    }
    m_controller_->DmrStop(m_current_dmr_);
}

void MainPage::PauseMedia(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    if (MediaPlayer->CanPause) {
        if (MediaPlayer->CurrentState == Windows::UI::Xaml::Media::MediaElementState::Playing
            || MediaPlayer->CurrentState == Windows::UI::Xaml::Media::MediaElementState::Stopped) {
                MediaPlayer->Pause();
        }
        m_controller_->DmrPause(m_current_dmr_);
    } else {
        ShowInfo->Text = "不支持拖动操作！";
    }
}

void MainPage::ProcessBar_ValueChanged(Platform::Object^ sender, Windows::UI::Xaml::Controls::Primitives::RangeBaseValueChangedEventArgs^ e)
{
    Windows::UI::Xaml::Controls::Slider^ slider = safe_cast<Windows::UI::Xaml::Controls::Slider^>(sender);
    if (!MediaPlayer->CanSeek) {
        slider->Value = 0;
    }
    Windows::Foundation::TimeSpan curpos;
    curpos.Duration = (slider->Value / 100.0) * MediaPlayer->NaturalDuration.TimeSpan.Duration;
    MediaPlayer->Position = curpos;
    m_controller_->DmrSeek(m_current_dmr_,  (uint64)(curpos.Duration / 10000000.0));
}

void MainPage::Slider_ValueChanged_1(Platform::Object^ sender, Windows::UI::Xaml::Controls::Primitives::RangeBaseValueChangedEventArgs^ e)
{
    Windows::UI::Xaml::Controls::Slider^ slider = safe_cast<Windows::UI::Xaml::Controls::Slider^>(sender);
    MediaPlayer->Volume = slider->Value / 100.0;
    m_controller_->DmrSetVolume(m_current_dmr_, (int32)slider->Value);
}

void MainPage::DmrItemListView_SelectionChanged(Platform::Object^ sender,
                                             Windows::UI::Xaml::Controls::SelectionChangedEventArgs^ e)
{
    DeviceInfo^ devinfo = safe_cast<DeviceInfo^>(DmrItemListView->SelectedItem);
    m_current_dmr_ = devinfo->DeviceId;
}

void MainPage::PageLoadedHandler(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    m_controller_->Start();
}
