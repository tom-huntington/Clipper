#pragma once

#include "MainWindow.g.h"

namespace winrt::Clipper::implementation
{
    struct MainWindow : MainWindowT<MainWindow>
    {
        MainWindow();

        winrt::fire_and_forget TextBoxPopup_KeyDown(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::Input::KeyRoutedEventArgs const& e);
        winrt::fire_and_forget TextBoxPopup2_KeyDown(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::Input::KeyRoutedEventArgs const& e);
        winrt::fire_and_forget Grid_Loaded(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);

        //std::optional<uint32_t> m_start_position;
        winrt::Windows::Storage::IStorageFile m_file;
        std::optional<winrt::Windows::Foundation::TimeSpan> m_start;
        std::optional<winrt::hstring> m_VideosPath;
        std::optional<winrt::Windows::Foundation::TimeSpan> m_end;
        uint32_t RetrieveInt(winrt::hstring key);
        winrt::Windows::Storage::StorageFolder m_outputFolder{nullptr};
        void Button_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        void Grid_KeyDown(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::Input::KeyRoutedEventArgs const& e);
        void Run_ffmpeg(std::wstringstream o, winrt::Windows::Foundation::TimeSpan start, winrt::Windows::Foundation::TimeSpan end);
        winrt::fire_and_forget FlashNotification(winrt::hstring title);
        void RunProcess(std::wstring_view cmd);
        void RunProcess2(std::wstring_view cmd);

        winrt::Windows::Foundation::IAsyncOperation<winrt::Windows::Storage::StorageFolder> PickOutputFolder();

        int m_playback_speed_key;
        winrt::Windows::Foundation::TimeSpan m_previous_update_pos{ 0 };
        winrt::Windows::Foundation::TimeSpan m_largest_update_pos{ 0 };
        void ShowTextBox();
        void RevertToClippingState();
        void SetInt(winrt::hstring key, int value);
        
        void UpdateOutputFolderToolTip();
    };
}

namespace winrt::Clipper::factory_implementation
{
    struct MainWindow : MainWindowT<MainWindow, implementation::MainWindow>
    {
    };
}
