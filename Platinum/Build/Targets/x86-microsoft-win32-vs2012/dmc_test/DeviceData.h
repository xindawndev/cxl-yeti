#pragma once

#include "pch.h"

namespace dmc_test {
    [Windows::UI::Xaml::Data::Bindable]
    public ref class DeviceInfo sealed {
    public:
        DeviceInfo(void) {}
        property Platform::String^ DeviceId;
        property Platform::String^ DeviceName;
    private:
        ~DeviceInfo(void) {}
    };

    [Windows::UI::Xaml::Data::Bindable]
    public ref class Devices sealed {
    public:
        Devices(void) {
            m_items = ref new Platform::Collections::Vector<DeviceInfo^>();
        }
        property Windows::Foundation::Collections::IVector<DeviceInfo^>^ Items {
            Windows::Foundation::Collections::IVector<DeviceInfo^>^ get() { return m_items; }
        }
    private:
        ~Devices(void) {}
        Platform::Collections::Vector<DeviceInfo^>^ m_items;
    };
}
