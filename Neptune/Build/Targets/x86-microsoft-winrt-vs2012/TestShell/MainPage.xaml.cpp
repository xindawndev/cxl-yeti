//
// MainPage.xaml.cpp
// Implementation of the MainPage class.
//

#include "pch.h"
#include "MainPage.xaml.h"

using namespace TestShell;

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

// The Blank Page item template is documented at http://go.microsoft.com/fwlink/?LinkId=234238

MainPage::MainPage()
{
	InitializeComponent();
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

extern int TimeTest1_main(int argc, char**argv);
extern int ThreadsTest1_main(int argc, char**argv);
extern int SocketTest1_main(int argc, char **argv);
extern int UdpTest1_main(int argc, char **argv);
extern int HttpClientTest1_main(int argc, char **argv);
extern int HttpClientTest2_main(int argc, char **argv);
extern int HttpServerTest1_main(int argc, char **argv);

void TestShell::MainPage::GoButton_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    char *argv[] = {"test","http://www.baidu.com"};
    HttpServerTest1_main(sizeof(argv) / strlen(argv[0]), argv);
    HttpClientTest2_main(sizeof(argv) / strlen(argv[0]), argv);
    HttpClientTest1_main(sizeof(argv) / strlen(argv[0]), argv);
    UdpTest1_main(sizeof(argv) / strlen(argv[0]), argv);
    SocketTest1_main(0, NULL);
	ThreadsTest1_main(0, NULL);
    TimeTest1_main(0, NULL);
}
