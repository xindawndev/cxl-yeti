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
    MediaPlayer->CurrentStateChanged += ref new Windows::UI::Xaml::RoutedEventHandler(this, &MainPage::OnPlayStateChange);
    MediaPlayer->BufferingProgressChanged += ref new Windows::UI::Xaml::RoutedEventHandler(this, &MainPage::OnBufferingProgressChanged);
    MediaPlayer->SeekCompleted += ref new Windows::UI::Xaml::RoutedEventHandler(this, &MainPage::OnSeekCompleted);
    MediaPlayer->VolumeChanged += ref new Windows::UI::Xaml::RoutedEventHandler(this, &MainPage::OnVolumeChanged);
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

/// <summary>
/// Invoked when this page is about to be displayed in a Frame.
/// </summary>
/// <param name="e">Event data that describes how this page was reached.  The Parameter
/// property is typically used to configure the page.</param>
void MainPage::OnNavigatedTo(NavigationEventArgs^ e)
{
	(void) e;	// Unused parameter
}

void dmc_test::MainPage::StartDmcBtnClicked(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    Platform::String^ play_str = MediaUrl->Text;
    if (play_str->IsEmpty()) {
        ShowInfo->Text = "Url为空，失败！";
        return;
    }

    Windows::Foundation::Uri^ url = ref new Windows::Foundation::Uri(play_str);
    MediaPlayer->Source = url;
    MediaPlayer->Play();
    VolumeBar->Value = MediaPlayer->Volume;
    ShowInfo->Text = "播放成功！URL = " + play_str;

    dmc = ref new MediaController();
    dmc->Run();
}

void dmc_test::MainPage::PlayMedia(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    if (MediaPlayer->CurrentState == Windows::UI::Xaml::Media::MediaElementState::Stopped
        || MediaPlayer->CurrentState == Windows::UI::Xaml::Media::MediaElementState::Paused) {
            MediaPlayer->Play();
    }
}

void dmc_test::MainPage::StopMedia(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    if (MediaPlayer->CurrentState == Windows::UI::Xaml::Media::MediaElementState::Playing
        || MediaPlayer->CurrentState == Windows::UI::Xaml::Media::MediaElementState::Paused) {
            MediaPlayer->Stop();
    }
}

void dmc_test::MainPage::PauseMedia(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    if (MediaPlayer->CanPause) {
        if (MediaPlayer->CurrentState == Windows::UI::Xaml::Media::MediaElementState::Playing
            || MediaPlayer->CurrentState == Windows::UI::Xaml::Media::MediaElementState::Stopped) {
                MediaPlayer->Pause();
        }
    } else {
        ShowInfo->Text = "不支持拖动操作！";
    }
}

void dmc_test::MainPage::ProcessBar_ValueChanged(Platform::Object^ sender, Windows::UI::Xaml::Controls::Primitives::RangeBaseValueChangedEventArgs^ e)
{
    Windows::UI::Xaml::Controls::Slider^ slider = safe_cast<Windows::UI::Xaml::Controls::Slider^>(sender);
    if (!MediaPlayer->CanSeek) {
        slider->Value = 0;
    }
    Windows::Foundation::TimeSpan curpos;
    curpos.Duration = (slider->Value / 100.0) * MediaPlayer->NaturalDuration.TimeSpan.Duration;
    MediaPlayer->Position = curpos;
}

void dmc_test::MainPage::Slider_ValueChanged_1(Platform::Object^ sender, Windows::UI::Xaml::Controls::Primitives::RangeBaseValueChangedEventArgs^ e)
{
    Windows::UI::Xaml::Controls::Slider^ slider = safe_cast<Windows::UI::Xaml::Controls::Slider^>(sender);
    MediaPlayer->Volume = slider->Value / 100.0;
}

void dmc_test::MainPage::AddCalc(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    Platform::String^ var_a = VarA->Text;
    Platform::String^ var_b = VarB->Text;
}

void dmc_test::MainPage::PrimeCheck(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
}
