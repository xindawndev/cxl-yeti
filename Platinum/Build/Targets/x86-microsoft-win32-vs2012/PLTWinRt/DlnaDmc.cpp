// Class1.cpp
#include "pch.h"
#include "DlnaDmc.h"
#include <ppltasks.h>
#include <concurrent_vector.h>

using namespace DlnaLib;
using namespace Platform;
using namespace concurrency;
using namespace Platform::Collections;
using namespace Windows::Foundation::Collections;
using namespace Windows::Networking::Sockets;
using namespace Windows::Storage::Streams;
using namespace Windows::Foundation;
using namespace Windows::UI::Core;

DlnaDmc::DlnaDmc()
{
}

Windows::Foundation::Collections::IVector<double>^ DlnaDmc::compute_result(double input)
{;
float numbers[] = {1.0, 10.0, 60.0, 100.0, 600.0, 10000.0};
array_view<float, 1> logs(6, numbers);
_compute_result_impl(logs);

auto res = ref new Vector<double>();
int len = safe_cast<int>(logs.extent.size());
for (int i = 0; i < len; ++i) {
    res->Append(logs[i]);
}
return res;
}

void DlnaDmc::_compute_result_impl(concurrency::array_view<float, 1>&)
{
    //parallel_for_each(
    //    logs.extent,
    //    [=](index<1> idx) restrict(amp) {
    //        logs[idx] = concurrency::fast_math::log10(logs[idx]);
    //});
}

Windows::Foundation::IAsyncOperationWithProgress<Windows::Foundation::Collections::IVector<int>^, double>^ DlnaDmc::get_primes_ordered(int first, int last)
{
    return create_async([this, first, last]
    (progress_reporter<double> reporter) -> IVector<int>^ {
        if (first < 0 || last < 0) {
            throw ref new InvalidArgumentException();
        }
        concurrent_vector<int> primes;
        long operation = 0;
        long range = last - first + 1;
        double last_persent = 0.0;

        parallel_for(first, last + 1, [this, &primes, &operation,
            range, &last_persent, reporter](int n) {
                double progress = 100.0 * InterlockedIncrement(&operation) / range;
                if (progress >= last_persent) {
                    reporter.report(progress);
                    last_persent += 1.0;
                }
                if (_is_prime(n)) {
                    primes.push_back(n);
                }
        });

        reporter.report(100.0);
        std::sort(begin(primes), end(primes), std::less<int>());
        IVector<int>^ results = ref new Vector<int>();
        std::for_each(std::begin(primes), std::end(primes), [&results](int prime) {
            results->Append(prime);
        });

        return results;
    });
}

bool DlnaDmc::_is_prime(int n)
{
    if (n < 2) return false;
    for (int i = 2; i < n; ++i) {
        if ((n % i) == 0) return false;
    }
    return true;
}

Windows::Foundation::IAsyncActionWithProgress<double>^ DlnaDmc::get_primes_unordered(int first, int last)
{
    auto window = Windows::UI::Core::CoreWindow::GetForCurrentThread();
    m_dispatcher_ = window->Dispatcher;

    return create_async([this, first, last](progress_reporter<double> reporter) {
        if (first < 0 || last < 0) {
            throw ref new InvalidArgumentException();
        }

        concurrent_vector<int> primes;
        long operation = 0;
        long range = last - first + 1;
        double last_percent = 0.0;
        parallel_for(first, last + 1, [this, &primes, &operation, range, &last_percent, reporter](int n) {
            double progress = 100.0 * InterlockedIncrement(&operation) / range;
            if (progress >= last_percent) {
                reporter.report(progress);
                last_percent += 1.0;
            }

            if (_is_prime(n)) {
                primes.push_back(n);
                m_dispatcher_->RunAsync(CoreDispatcherPriority::Normal,
                    ref new DispatchedHandler([this, n]() {
                        this->primeFoundEvent(n);
                }, Platform::CallbackContext::Any));
            }
        });

        reporter.report(100.0);
    });
}

Windows::Foundation::IAsyncAction^ DlnaDmc::start_dmc(bool ignore_local)
{
    return create_async([this, ignore_local]() {
        DatagramSocket^ listener = ref new DatagramSocket();
        listener->MessageReceived += ref new TypedEventHandler<DatagramSocket^, DatagramSocketMessageReceivedEventArgs^>(this, &DlnaDmc::OnMessage);

        task<void>(listener->BindServiceNameAsync("1900")).then([this, listener] (task<void> previousTask) {
            try {
                previousTask.get();
                Windows::Networking::HostName^ hostname = ref new Windows::Networking::HostName("239.255.255.250");
                listener-> JoinMulticastGroup(hostname);
                task<IOutputStream^>(listener->GetOutputStreamAsync(hostname,"1900")).then([this] (IOutputStream^ stream) {
                    String^ stringToSend = "M-SEARCH * HTTP/1.1\r\n" +
                        "HOST: 239.255.255.250:1900\r\n" +
                        "ST: upnp:rootdevice\r\n" +
                        "MAN: \"ssdp:discover\"\r\n" +
                        "MX: 333\r\n\r\n";
                    DataWriter^ writer = ref new DataWriter(stream);
                    task<unsigned int>(writer->StoreAsync()).then([this, stringToSend] (task<unsigned int> writeTask) {
                        try {
                            // Try getting an excpetion.
                            writeTask.get();
                        } catch (Exception^ exception) {
                        }
                    });
                });

                //String^ stringToSend = "M-SEARCH * HTTP/1.1\r\n" +
                //    "HOST: 239.255.255.250:1900\r\n" +
                //    "ST: upnp:rootdevice\r\n" +
                //    "MAN: \"ssdp:discover\"\r\n" +
                //    "MX: 333\r\n\r\n";
                //DataWriter^ writer = ref new DataWriter(listener->OutputStream);
                //task<unsigned int>(writer->StoreAsync()).then([this, stringToSend] (task<unsigned int> writeTask) {
                //    try {
                //        // Try getting an excpetion.
                //        writeTask.get();
                //    } catch (Exception^ exception) {
                //    }
                //});
            } catch (Exception^ exception) {
            }
        });

    });
}

void DlnaDmc::OnMessage(DatagramSocket^ socket, DatagramSocketMessageReceivedEventArgs^ eventArguments)
{
    //auto window = Windows::UI::Core::CoreWindow::GetForCurrentThread();
    //m_dispatcher_ = window->Dispatcher;
    //m_dispatcher_->RunAsync(CoreDispatcherPriority::Normal,
    //    ref new DispatchedHandler([this]() {
    //        this->onSetUri(222);
    //}, Platform::CallbackContext::Any));
    Platform::String^ dev("Device Add");
    this->onDeviceAdd(dev);
}
