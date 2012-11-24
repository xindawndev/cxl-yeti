Duration
Duration
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
    m_controller_->onDmrSetAVTransportURI += ref new OnDmrSetAVTransportURI(this, &MainPage::On_Common);
    m_controller_->onDmrPlay += ref new OnDmrPlay(this, &MainPage::On_Common);
    m_controller_->onDmrSeek += ref new OnDmrSeek(this, &MainPage::On_Common);
    m_controller_->onDmrPause += ref new OnDmrPause(this, &MainPage::On_Common);
    m_controller_->onDmrStop += ref new OnDmrStop(this, &MainPage::On_Common);
    m_controller_->onDmrSetMute += ref new OnDmrSetMute(this, &MainPage::On_Common);
    m_controller_->onDmrSetVolume += ref new OnDmrSetVolume(this, &MainPage::On_Common);
    m_controller_->onDmrGetPositionInfo += ref new OnDmrGetPositionInfo(this, &MainPage::On_GetPositionInfo);
    m_controller_->onDmrGetVolume += ref new OnDmrGetVolume(this, &MainPage::On_GetVolume);
    m_controller_->onDmrGetDeviceCapabilities += ref new OnDmrGetDeviceCapabilities(this, &MainPage::On_GetCap);
    m_controller_->onDmrGetTransportInfo += ref new OnDmrGetTransportInfo(this, &MainPage::On_GetTrans);
    m_controller_->onDmrGetMute += ref new OnDmrGetMute(this, &MainPage::On_GetMute);
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

    if (is_dmr) {
        this->Dispatcher->RunAsync(CoreDispatcherPriority::Normal, ref new DispatchedHandler([this, device_id, divice_name] () {
            DeviceInfo^ di = ref new DeviceInfo();
            di->DeviceId = device_id;
            di->DeviceName = divice_name;
            m_dmrs_->Items->Append(di);
            ShowInfo->Text += "Add DMR: " + device_id + " Name: " + divice_name + "\r\n";
        }));
    } else {
        this->Dispatcher->RunAsync(CoreDispatcherPriority::Normal, ref new DispatchedHandler([this, device_id, divice_name] () {
            DeviceInfo^ di = ref new DeviceInfo();
            di->DeviceId = device_id;
            di->DeviceName = divice_name;
            m_dmss_->Items->Append(di);
            ShowInfo->Text += "Add DMS: " + device_id + " Name: " + divice_name + "\r\n";
        }));
    }
}

void MainPage::On_DeviceDel(Platform::String^ device_id, Platform::String^ divice_name, bool is_dmr)
{
    if (is_dmr) {
        this->Dispatcher->RunAsync(CoreDispatcherPriority::Normal, ref new DispatchedHandler([this, device_id, divice_name] () {
            for (int i = 0; i < (int)m_dmrs_->Items->Size; ++i) {

                if (m_dmrs_->Items->GetAt(i)->DeviceId == device_id && 
                    m_dmrs_->Items->GetAt(i)->DeviceName == divice_name)
                    m_dmrs_->Items->RemoveAt(i);
            }
            ShowInfo->Text += "Remove DMR: " + device_id + " Name: " + divice_name + "\r\n";
        }));
    } else {
        this->Dispatcher->RunAsync(CoreDispatcherPriority::Normal, ref new DispatchedHandler([this, device_id, divice_name] () {
            for (int i = 0; i < (int)m_dmss_->Items->Size; ++i) {
                if (m_dmss_->Items->GetAt(i)->DeviceId == device_id && 
                    m_dmss_->Items->GetAt(i)->DeviceName == divice_name)
                    m_dmss_->Items->RemoveAt(i);
            }
            ShowInfo->Text += "Remove DMS: " + device_id + " Name: " + divice_name + "\r\n";
        }));
    }
}

void MainPage::On_Common(PLTWinRt::ErrorCode ec, Platform::String^ device_id)
{
    this->Dispatcher->RunAsync(CoreDispatcherPriority::Normal, ref new DispatchedHandler([this, device_id, ec] () {
        ShowInfo->Text = "DMR: " + device_id + " Error Code: " + ec.ToString() + "\r\n";
    }));
}


void MainPage::On_GetPositionInfo(PLTWinRt::ErrorCode ec, Platform::String^ device_id, PLTWinRt::DMR_PositionInfo^ pi)
{
    this->Dispatcher->RunAsync(CoreDispatcherPriority::Normal, ref new DispatchedHandler([this, device_id, pi] () {
        if (pi != nullptr)
            ShowInfo->Text = "DMR: " + device_id + " Duration: " + pi->track_duration.Duration + " Position: " + pi->abs_time.Duration + "\r\n";
    }));
}

void MainPage::On_GetVolume(PLTWinRt::ErrorCode ec, Platform::String^ device_id, unsigned int vol)
{
    this->Dispatcher->RunAsync(CoreDispatcherPriority::Normal, ref new DispatchedHandler([this, device_id, vol] () {
        ShowInfo->Text = "\r\nDMR: " + device_id + " Current Volume: " + vol + "\r\n";
    }));
}

void MainPage::On_GetCap(PLTWinRt::ErrorCode ec, Platform::String^ device_id, Windows::Foundation::Collections::IVector<Platform::String^>^ pm, Windows::Foundation::Collections::IVector<Platform::String^>^ cm, Windows::Foundation::Collections::IVector<Platform::String^>^ cqm)
{
    this->Dispatcher->RunAsync(CoreDispatcherPriority::Normal, ref new DispatchedHandler([this, device_id, pm] () {
        if (pm != nullptr) {
            ShowInfo->Text = "\r\nDMR: " + device_id + " Caps: ";
            for (uint32 i = 0; i < pm->Size; ++i) {
                ShowInfo->Text += pm->GetAt(i) + " ";
            }
            ShowInfo->Text += "\r\n";
        }
    }));
}

void MainPage::On_GetTrans(PLTWinRt::ErrorCode ec, Platform::String^ device_id, PLTWinRt::DMR_TransportInfo^ trans)
{
    this->Dispatcher->RunAsync(CoreDispatcherPriority::Normal, ref new DispatchedHandler([this, device_id, trans] () {
        if (trans != nullptr) {
            ShowInfo->Text = "\r\nDMR: " + device_id + " cur_transport_state: " + trans->cur_transport_state + " cur_transport_status: " + trans->cur_transport_status + " cur_speed: " + trans->cur_speed;
        }
    }));
}

void MainPage::On_GetMute(PLTWinRt::ErrorCode ec, Platform::String^ device_id,bool is_mute)
{
    this->Dispatcher->RunAsync(CoreDispatcherPriority::Normal, ref new DispatchedHandler([this, device_id, is_mute] () {
        ShowInfo->Text = "\r\nDMR: " + device_id + " is mute: " + is_mute + "\r\n";
    }));
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

    //m_controller_->DmrSetAVTransportURI(m_current_dmr_, play_str, nullptr);
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
    curpos.Duration = (long long)((slider->Value / 100.0) * MediaPlayer->NaturalDuration.TimeSpan.Duration);
    MediaPlayer->Position = curpos;
    m_controller_->DmrSeek(m_current_dmr_,  (uint64)(curpos.Duration / 10000000.0));
}

void MainPage::Slider_ValueChanged_1(Platform::Object^ sender, Windows::UI::Xaml::Controls::Primitives::RangeBaseValueChangedEventArgs^ e)
{
    Windows::UI::Xaml::Controls::Slider^ slider = safe_cast<Windows::UI::Xaml::Controls::Slider^>(sender);
    MediaPlayer->Volume = slider->Value / 100.0;
    m_controller_->DmrSetVolume(m_current_dmr_, (int32)slider->Value);
    m_controller_->DmrSetMute(m_current_dmr_, slider->Value == 0);
}

void MainPage::DmrItemListView_SelectionChanged(Platform::Object^ sender,
                                             Windows::UI::Xaml::Controls::SelectionChangedEventArgs^ e)
{
    DeviceInfo^ devinfo = safe_cast<DeviceInfo^>(DmrItemListView->SelectedItem);
    m_current_dmr_ = devinfo->DeviceId;
    Platform::String^ play_str = MediaUrl->Text;
    if (play_str->IsEmpty()) {
        ShowInfo->Text = "Url为空，失败！";
        return;
    }

    m_controller_->DmrSetAVTransportURI(m_current_dmr_, play_str, nullptr);
    m_controller_->DmrPlay(m_current_dmr_);
}

void MainPage::PageLoadedHandler(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    m_controller_->Start();
    this->DataContext = m_dmrs_;
}

void dmc_test::MainPage::OnGetPosClicked(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    m_controller_->DmrGetPositionInfo(m_current_dmr_);
}

void dmc_test::MainPage::OnGetVolumeClicked(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    m_controller_->DmrGetVolume(m_current_dmr_);
}

void dmc_test::MainPage::OnGetCapClicked(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    m_controller_->DmrGetDeviceCapabilities(m_current_dmr_);
}

void dmc_test::MainPage::OnGetTransClicked(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    m_controller_->DmrGetTransportInfo(m_current_dmr_);
}

void dmc_test::MainPage::OnGetMuteClicked(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    m_controller_->DmrGetMute(m_current_dmr_);
}
