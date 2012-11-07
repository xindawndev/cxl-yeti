#pragma once
#include <collection.h>
#include <amp.h>
#include <amp_math.h>

namespace DlnaLib
{
    // 添加委托
    public delegate void PrimeFoundHandler(int i);
    // 设备添加
    public delegate void OnDeviceAdd(Platform::String^ divice_name);
    // 设备删除
    public delegate void OnDeviceDel(Platform::String^ divice_name);
    public delegate void OnGetDevCap(int ec, Platform::String^ play_media, Platform::String^ rec_media, Platform::String^ rec_qua_meida);
    public delegate void OnPlay(int ec);
    public delegate void OnSeek(int ec);
    public delegate void OnStop(int ec);
    public delegate void OnPause(int ec);
    public delegate void OnNext(int ec);
    public delegate void OnPrev(int ec);
    public delegate void OnSetUri(int ec);
    public delegate void OnSetVol(int ec);
    public delegate void OnSetMute(int ec);
    public delegate void OnSetPlayMode(int ec);
    public delegate void OnGetMediaInfo(int ec);
    public delegate void OnGetPosition(int ec);
    public delegate void OnGetTransportInfo(int ec);

    public ref class DlnaDmc sealed
    {
    public: 
        DlnaDmc();

    public:
        // 测试：同步方法
        Windows::Foundation::Collections::IVector<double>^ compute_result(double input);
        // 测试：异步方法
        Windows::Foundation::IAsyncOperationWithProgress<Windows::Foundation::Collections::IVector<int>^, double>^ get_primes_ordered(int first, int last);
        // 测试：激发事件，提供进度信息
        Windows::Foundation::IAsyncActionWithProgress<double>^ get_primes_unordered(int first, int last);
        // 测试：启动DMC
        Windows::Foundation::IAsyncAction^ start_dmc(bool ignore_local);

    public: // Events
        event PrimeFoundHandler^    primeFoundEvent;
        event OnDeviceAdd^          onDeviceAdd;
        event OnDeviceDel^          onDeviceDel;
        event OnGetDevCap^          onGetDevCap;
        event OnPlay^               onPlay;
        event OnSeek^               onSeek;
        event OnStop^               onStop;
        event OnPause^              onPause;
        event OnNext^               onNext;
        event OnPrev^               onPrev;
        event OnSetUri^             onSetUri;
        event OnSetVol^             onSetVol;
        event OnSetMute^            onSetMute;
        event OnSetPlayMode^        onSetPlayMode;
        event OnGetMediaInfo^       onGetMediaInfo;
        event OnGetPosition^        onGetPosition;
        event OnGetTransportInfo^   onGetTransportInfo;
    private:
        void _compute_result_impl(concurrency::array_view<float, 1>&);
        bool _is_prime(int n);
        void OnMessage(Windows::Networking::Sockets::DatagramSocket^ socket, Windows::Networking::Sockets::DatagramSocketMessageReceivedEventArgs^ eventArguments);

    private:
        Windows::UI::Core::CoreDispatcher^ m_dispatcher_;
    };
}