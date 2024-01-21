#include "pch.h"
#include "MainWindow.xaml.h"
#if __has_include("MainWindow.g.cpp")
#include "MainWindow.g.cpp"
#endif

#include <dwmapi.h>

using namespace winrt;
using namespace Microsoft::UI::Xaml;

constexpr auto KeyOutputFolderPath = L"m_outputFolder::Path";

// To learn more about WinUI, the WinUI project structure,
// and more about our project templates, see: http://aka.ms/winui-project-info.

winrt::Clipper::implementation::MainWindow::MainWindow()
{
    // Xaml objects should not call InitializeComponent during construction.
    // See https://github.com/microsoft/cppwinrt/tree/master/nuget#initializecomponent

#if 0
    using namespace Windows::UI::ViewManagement;
    auto settings = UISettings();
    constexpr auto IsColorLight = [](Windows::UI::Color& clr) -> bool
        {
            return (((5 * clr.G) + (2 * clr.R) + clr.B) > (8 * 128));
        };
    auto foreground = settings.GetColorValue(UIColorType::Foreground);
    if (IsColorLight(foreground))
    {
        BOOL value = TRUE;
        HWND hwnd;
        auto window = this->try_as<winrt::Microsoft::UI::Xaml::Window>();
        winrt::check_hresult(window.as<IWindowNative>()->get_WindowHandle(&hwnd));
        ::DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &value, sizeof(value));
    }
#endif
}

auto durationToString(const winrt::Windows::Foundation::TimeSpan& duration) {
    using namespace std::chrono;

    // Convert duration to minutes and seconds
    auto minutes = duration_cast<std::chrono::minutes>(duration);
    auto seconds = duration_cast<std::chrono::seconds>(duration - minutes);

    // Format minutes and seconds using std::format
    //return winrt::to_hstring(std::format(L"{:02}:{:02}", minutes.count(), seconds.count()));
    return winrt::format(L"{}:{:02}", minutes.count(), seconds.count());
}

winrt::fire_and_forget winrt::Clipper::implementation::MainWindow::Grid_Loaded(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e)
{
    


    const DWORD bufferSize = MAX_PATH;
    char filePath[MAX_PATH];
    DWORD result = SearchPathA(nullptr, "ffmpeg.exe", nullptr, bufferSize, filePath, nullptr);
    if (result == 0)
    {
        auto dialog = winrt::Microsoft::UI::Xaml::Controls::ContentDialog{};
        dialog.XamlRoot(Content().XamlRoot());
        dialog.Title(winrt::box_value(L"ffmpeg.exe not found in path"));
        //dialog.Content(winrt::box_value(L"Download https://ffmpeg.org/download.html and place into C:\\Windows"));

        const wchar_t* xamlMarkup = LR"(
    <TextBlock xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>
        <Span>
            <Run>1) Download FFmpeg </Run>
            <Hyperlink NavigateUri='https://ffmpeg.org/download.html'>here</Hyperlink>
            <LineBreak/>
            <Run>2) Drag and drop into C:\Windows</Run>
        </Span>
    </TextBlock>
)";
        auto uiElement = winrt::Microsoft::UI::Xaml::Markup::XamlReader::Load(xamlMarkup);

        dialog.Content(uiElement);
        dialog.CloseButtonText(L"Shutdown");
        co_await dialog.ShowAsync();
        Close();
        co_return;
    }


    //using namespace winrt::Windows::Storage;
    //m_outputFolder = KnownFolders::VideosLibrary();
    
    using namespace winrt::Windows::Storage::Pickers;
    FileOpenPicker picker{};
    auto windowNative{ this->m_inner.as<::IWindowNative>() };
    HWND hWnd{ 0 };
    windowNative->get_WindowHandle(&hWnd);
    auto initializeWithWindow{ picker.as<::IInitializeWithWindow>() };
    initializeWithWindow->Initialize(hWnd);
    picker.SuggestedStartLocation(PickerLocationId::VideosLibrary);
    picker.FileTypeFilter().Append(L".mp4");
    picker.CommitButtonText(L"Clip this file");
    m_file = co_await picker.PickSingleFileAsync();

    if (!m_file)
    {
        Close();
        co_return;
    }

    using namespace Windows::Storage;
    auto values = ApplicationData::Current().LocalSettings().Values();
    auto outputPath = values.TryLookup(KeyOutputFolderPath);
    if (outputPath)
    {
        m_outputFolder = co_await StorageFolder::GetFolderFromPathAsync(outputPath.as<winrt::hstring>());
        UpdateOutputFolderToolTip();
    }
    else
    {
        auto folder = co_await PickOutputFolder();
        if (not folder)
        {
            Close();
            co_return;
        }
        
    }


    Content().Focus(FocusState::Programmatic);

    

    using namespace winrt::Windows::Media::Core;
    auto mediaSource = MediaSource::CreateFromStorageFile(m_file);
    mediaPlayerElement().Source(mediaSource);


    
#if 1
    mediaPlayerElement().MediaPlayer().PlaybackSession().PositionChanged([=](auto session, auto) {
        if (not (session.Position() - m_previous_update_pos >= std::chrono::seconds(1))) return;
        m_largest_update_pos = (std::max)(m_largest_update_pos, session.Position());

        DispatcherQueue().TryEnqueue(winrt::Microsoft::UI::Dispatching::DispatcherQueuePriority::Low, [=]() {
            if (m_start)
            {
                DurationTextBlock().Visibility(Visibility::Visible);
                m_previous_update_pos = session.Position();
                DurationTextBlock().Text(durationToString(m_previous_update_pos - *m_start));
            }
            else
            {
                DurationTextBlock().Visibility(Visibility::Collapsed);
            }
            });
        });
#endif

    Closed([this](auto, auto) {
        auto a = mediaPlayerElement().MediaPlayer().Position();
        SetInt(m_file.Path(), std::chrono::duration_cast<std::chrono::seconds>(a).count());
        });

    auto start = RetrieveInt(m_file.Path());
    mediaPlayerElement().MediaPlayer().Position(std::chrono::seconds(start));

    
}


uint32_t winrt::Clipper::implementation::MainWindow::RetrieveInt(winrt::hstring key)
{
    using namespace Windows::Storage;
    auto values = ApplicationData::Current().LocalSettings().Values();
    if (values.HasKey(key))
    {
        auto a = values.Lookup(key).as<int>();
        if (a > 0) return a;
    }
    return 0;
}

void winrt::Clipper::implementation::MainWindow::Button_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e)
{
    PickOutputFolder();
}

void winrt::Clipper::implementation::MainWindow::ShowTextBox()
{
    if (not m_start)
    {
        FlashNotification(L"Press I to mark clip start");
        return;
    }
    if (not m_end)
    {
        FlashNotification(L"Press O to mark clip end");
        return;
    }
    if (*m_start > *m_end)
    {
        FlashNotification(L"Clip start must be less than end");
        return;
    }

    mediaPlayerElement().MediaPlayer().Pause();
    PopUp().Visibility(Visibility::Visible);
    TextBoxPopup().Focus(FocusState::Programmatic);
}


void winrt::Clipper::implementation::MainWindow::Grid_KeyDown(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::Input::KeyRoutedEventArgs const& e)
{
    using namespace Windows::System;
    switch (e.Key())
    {
        break;
        case VirtualKey::J:
        case VirtualKey::L:
        {
            auto mediaPlayer = mediaPlayerElement().MediaPlayer();
            auto position = mediaPlayer.Position();
            
            using namespace winrt::Windows::UI::Core;
            using namespace winrt::Microsoft::UI::Input;
            auto key_state = InputKeyboardSource::GetKeyStateForCurrentThread(VirtualKey::Shift);
            bool isShiftPressed = key_state == CoreVirtualKeyStates::Down;

            auto delta = isShiftPressed ? 2U : 5U;
            auto new_pos = [&]() {
                if (e.Key() == VirtualKey::J)
                    return position - std::chrono::seconds(delta);
                else return position + std::chrono::seconds(delta);
                }();
            mediaPlayer.Position(new_pos);
        }
        break; case VirtualKey::O:
        {
            m_end = mediaPlayerElement().MediaPlayer().Position();
            e.Handled(true);
            ShowTextBox();
        }
        break; case VirtualKey::Enter:
        {
            e.Handled(true);
            ShowTextBox();
        }
        break; case VirtualKey::I:
        {
            m_start = mediaPlayerElement().MediaPlayer().Position();
            //FlashNotification(L"Clip start selected");
            e.Handled(true);
            ShowTextBox();
        }
        break;
        case static_cast<VirtualKey>(190):
        case static_cast<VirtualKey>(188):
        {
            auto delta = e.Key() == static_cast<VirtualKey>(190) ? 1 : -1;
            using namespace winrt::Windows::UI::Core;
            using namespace winrt::Microsoft::UI::Input;
            auto key_state = InputKeyboardSource::GetKeyStateForCurrentThread(VirtualKey::Shift);
            bool isShiftPressed = key_state == CoreVirtualKeyStates::Down;
            //if (not isShiftPressed) { OutputDebugStringW(L"Not Shift"); co_return; };
            constexpr auto mapping = std::array{1.0, 1.25, 1.5, 1.75, 2.0, 2.5, 3.0};
            m_playback_speed_key = std::clamp(m_playback_speed_key + delta, 0, int(mapping.size()-1));
            mediaPlayerElement().MediaPlayer().PlaybackSession().PlaybackRate(mapping[m_playback_speed_key]);
            PlaybackSpeedTextBlock().Text(winrt::format(L"\u00D7{}", mapping[m_playback_speed_key]));
        }
        break; case VirtualKey::H:
        {
            mediaPlayerElement().MediaPlayer().PlaybackSession().Position(m_largest_update_pos);
            FlashNotification(L"Jumped to furtherest reached");

        }
        break; case VirtualKey::S:
        {
            if (not m_start) return;
            mediaPlayerElement().MediaPlayer().PlaybackSession().Position(*m_start);
            FlashNotification(L"Jumped to start marker");
        }
        break; case VirtualKey::E:
        {
            if (not m_end) return;
            mediaPlayerElement().MediaPlayer().PlaybackSession().Position(*m_end);
            FlashNotification(L"Jumped to end marker");
        }
        break; case VirtualKey::Escape:
        {
            Content().Focus(FocusState::Programmatic);
        }
    }


}

void winrt::Clipper::implementation::MainWindow::RevertToClippingState()
{
    mediaPlayerElement().MediaPlayer().Play();
    PopUp().Visibility(Visibility::Collapsed);
    TextBoxPopup().Text(L"");
    Content().Focus(FocusState::Programmatic);
}

winrt::fire_and_forget winrt::Clipper::implementation::MainWindow::TextBoxPopup_KeyDown(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::Input::KeyRoutedEventArgs const& e)
{
    using namespace Windows::System;
    e.Handled(true);
    if (e.Key() == VirtualKey::Escape)
    {
        RevertToClippingState();
        co_return;
    }

    if (e.Key() not_eq VirtualKey::Enter) co_return;

    std::wstringstream o;
    o << TextBoxPopup().Text().c_str() << L".mp4";
    if (co_await m_outputFolder.TryGetItemAsync(o.str().c_str()))
    {
        auto tip = winrt::Microsoft::UI::Xaml::Controls::ToolTip{};
        tip.Content(winrt::box_value(winrt::hstring{ L"File already exists!" }));
        winrt::Microsoft::UI::Xaml::Controls::ToolTipService::SetToolTip(TextBoxPopup(), tip);
        tip.IsOpen(true);
        co_await winrt::resume_after(std::chrono::seconds(1));
        co_await wil::resume_foreground(DispatcherQueue());
        tip.IsOpen(false);
        winrt::Microsoft::UI::Xaml::Controls::ToolTipService::SetToolTip(TextBoxPopup(), nullptr);
        co_return;
    }

    RevertToClippingState();
    Run_ffmpeg(std::move(o), *m_start, *m_end);
    m_start = std::nullopt;
    m_end = std::nullopt;
}

std::wstring TimeStamp(winrt::Windows::Foundation::TimeSpan timeStamp)
{
    auto position = timeStamp;
    //auto count = position.count();
    //auto total_seconds = (position.count() / 10000000);
    auto seconds = (position.count() / 10000000) % 60;
    //auto total_minutes = (position.count() / (10000000 * 60));
    auto minutes = (position.count() / (10000000 * 60)) % 60;
    auto hours = (position.count() / (10000000 * 60)) / 60;
    //auto hours2 = total_minutes / 60;

    return std::format(L"{:02}:{:02}:{:02}", hours, minutes, seconds);
}

void winrt::Clipper::implementation::MainWindow::Run_ffmpeg(std::wstringstream o, winrt::Windows::Foundation::TimeSpan start, winrt::Windows::Foundation::TimeSpan end)
{
    auto start_timestamp = TimeStamp(start);
    auto length_timestamp = TimeStamp(end - start + winrt::Windows::Foundation::TimeSpan{ 10000000 / 4 });


    /*std::wstringstream cmdLineVideo;
    cmdLineVideo << L"ffmpeg -ss " << start_timestamp;
    cmdLineVideo << L" -i \"" << m_path.c_str() << L"\"";
    cmdLineVideo << L" -t " << length_timestamp;
    cmdLineVideo << L" -c copy \"C:\\Users\\a\\AppData\\Local\\Packages\\809b67aa-05df-44ec-adbb-e70c4677cdb7_2zbdsnss4mwk0\\LocalState\\Video\\";
    cmdLineVideo << o.str().c_str() << L"\"";*/


    using namespace std::literals;
    //auto outputPath = [&]() -> winrt::hstring {
    //    if (m_outputFolder.DisplayName() == L"Videos"sv)
    //    {
    //        if (not m_VideosPath)
    //        {
    //            IKnownFolderManager* pKnownFolderManager;
    //            winrt::check_hresult(CoCreateInstance(CLSID_KnownFolderManager, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pKnownFolderManager)));
    //            IKnownFolder* pDownloadsFolder;
    //            winrt::check_hresult(pKnownFolderManager->GetFolder(FOLDERID_Videos, &pDownloadsFolder));
    //            PWSTR pszFolderPath;
    //            winrt::check_hresult(pDownloadsFolder->GetPath(0, &pszFolderPath));
    //            m_VideosPath = winrt::hstring{ pszFolderPath };
    //        }
    //        return *m_VideosPath;
    //    }
    //    else return m_outputFolder.Path();
    //    }();
    auto outputPath = m_outputFolder.Path();
    winrt::check_bool(not outputPath.empty());
    auto cmdLineVideo = std::format(L"ffmpeg -ss {} -i \"{}\" -t {} -c copy \"{}\\{}\" ", start_timestamp, m_file.Path(), length_timestamp, outputPath, o.view());
    //OutputDebugString(cmdLineVideo.str().c_str());


    RunProcess(cmdLineVideo.data());

}

winrt::fire_and_forget winrt::Clipper::implementation::MainWindow::FlashNotification(winrt::hstring title)
{
    Notification().Title(title);
    Notification().IsOpen(true);
    co_await winrt::resume_after(std::chrono::seconds(1));
    co_await wil::resume_foreground(DispatcherQueue());
    Notification().IsOpen(false);
}

void winrt::Clipper::implementation::MainWindow::UpdateOutputFolderToolTip()
{
    using namespace winrt::Microsoft::UI::Xaml::Controls;
    auto toolTip = ToolTip();
    toolTip.Content(winrt::box_value(L"Output Folder: " + m_outputFolder.Path()));
    ToolTipService::SetToolTip(PickFolderButton(), toolTip);
}


winrt::Windows::Foundation::IAsyncOperation<winrt::Windows::Storage::StorageFolder> winrt::Clipper::implementation::MainWindow::PickOutputFolder()
{
    using namespace winrt::Windows::Storage::Pickers;
    FolderPicker picker{};
    HWND hWnd{ 0 };
    m_inner.as<::IWindowNative>()->get_WindowHandle(&hWnd);
    picker.as<::IInitializeWithWindow>()->Initialize(hWnd);
    picker.SuggestedStartLocation(PickerLocationId::VideosLibrary);
    picker.CommitButtonText(L"Save clips here");
    auto folder = co_await picker.PickSingleFolderAsync();
    if (not folder) co_return folder;

    winrt::check_bool(not folder.Path().empty());

    m_outputFolder = folder;
    

    using namespace Windows::Storage;
    auto values = ApplicationData::Current().LocalSettings().Values();
    values.Insert(KeyOutputFolderPath, winrt::box_value(folder.Path()));
    co_return folder;
}

auto ReadChildProcessOutput(HANDLE hPipe)
{
    const int bufferSize = 4096; // You can adjust the buffer size as needed
    std::vector<char> buffer(bufferSize, 0);
    std::string output;

    while (true)
    {
        DWORD bytesRead = 0;
        if (!ReadFile(hPipe, buffer.data(), bufferSize, &bytesRead, NULL) || bytesRead == 0)
        {
            break; // No more data or an error occurred
        }

        output.append(buffer.data(), bytesRead);
    }

    return output;
}

void winrt::Clipper::implementation::MainWindow::RunProcess(std::wstring_view  cmd)
{
    HANDLE g_hChildStd_OUT_Rd = NULL;
    HANDLE g_hChildStd_OUT_Wr = NULL;

    SECURITY_ATTRIBUTES saAttr{};
    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    saAttr.bInheritHandle = TRUE;
    saAttr.lpSecurityDescriptor = NULL;

    winrt::check_bool(CreatePipe(&g_hChildStd_OUT_Rd, &g_hChildStd_OUT_Wr, &saAttr, 0));

    // Ensure the read handle to the pipe for STDOUT is not inherited.
    winrt::check_bool(SetHandleInformation(g_hChildStd_OUT_Rd, HANDLE_FLAG_INHERIT, 0));


    PROCESS_INFORMATION piProcInfo{};
    STARTUPINFO siStartInfo{};
    siStartInfo.cb = sizeof(STARTUPINFO);
    siStartInfo.hStdError = g_hChildStd_OUT_Wr;
    siStartInfo.hStdOutput = g_hChildStd_OUT_Wr;
    siStartInfo.dwFlags |= STARTF_USESTDHANDLES;

    OutputDebugString(cmd.data());

    auto success = CreateProcessW(NULL,
        const_cast<wchar_t*>(cmd.data()),     // command line 
        NULL,          // process security attributes 
        NULL,          // primary thread security attributes 
        TRUE,          // handles are inherited 
        CREATE_NO_WINDOW,             // creation flags CREATE_NEW_CONSOLE
        NULL,          // use parent's environment 
        NULL,          // use parent's current directory 
        &siStartInfo,  // STARTUPINFO pointer 
        &piProcInfo);  // receives PROCESS_INFORMATION
    CloseHandle(g_hChildStd_OUT_Wr);
    g_hChildStd_OUT_Wr = 0;


    if (not success)
    {
        auto std_out = winrt::to_hstring(ReadChildProcessOutput(g_hChildStd_OUT_Rd));
        OutputDebugString(std_out.c_str());
        auto dialog = winrt::Microsoft::UI::Xaml::Controls::ContentDialog{};
        dialog.XamlRoot(Content().XamlRoot());
        dialog.Title(winrt::box_value(L"FFmpeg returned non-zero exit code"));
        //auto content = std::format(L"Got this url from clipboard:\n{}", text);
        auto scrollViewer = winrt::Microsoft::UI::Xaml::Controls::ScrollViewer{};
        scrollViewer.HorizontalScrollBarVisibility(winrt::Microsoft::UI::Xaml::Controls::ScrollBarVisibility::Auto);
        scrollViewer.VerticalScrollBarVisibility(winrt::Microsoft::UI::Xaml::Controls::ScrollBarVisibility::Auto);
        auto textBlock = winrt::Microsoft::UI::Xaml::Controls::TextBlock{};
        textBlock.Text(winrt::to_hstring(winrt::format(L"> {}\n\n{}", cmd, std_out)));
        // Set the content of the ScrollViewer
        scrollViewer.Content(winrt::box_value(textBlock));
        dialog.CloseButtonText(L"Close");
        dialog.Content(winrt::box_value(scrollViewer));
        dialog.ShowAsync();
    }
}


void winrt::Clipper::implementation::MainWindow::SetInt(winrt::hstring key, int value)
{
    using namespace Windows::Storage;
    auto values = ApplicationData::Current().LocalSettings().Values();
    values.Insert(key, box_value(value));
}