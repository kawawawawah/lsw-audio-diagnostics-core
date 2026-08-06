// SPDX-FileCopyrightText: 2026 HIROAKI KAWAKITA
// SPDX-License-Identifier: MIT

#include "lsw/audio_diag/analyzer.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cwchar>
#include <limits>

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

namespace
{
    constexpr UINT_PTR timerIdentifier = 1U;
    constexpr UINT timerIntervalMilliseconds = 30U;
    constexpr std::size_t blockSize = 128U;
    constexpr double sampleRate = 48000.0;
    constexpr double pi = 3.14159265358979323846;

    enum class Scenario : int
    {
        cleanStereo = 1,
        dropout,
        sustainedClip,
        dcFault,
        invalidBurst,
        reversedPolarity,
        faultShowcase
    };

    [[nodiscard]] const wchar_t* scenarioName(const Scenario scenario) noexcept
    {
        switch (scenario)
        {
            case Scenario::cleanStereo: return L"Clean Stereo";
            case Scenario::dropout: return L"Dropout";
            case Scenario::sustainedClip: return L"Sustained Clip";
            case Scenario::dcFault: return L"DC Fault";
            case Scenario::invalidBurst: return L"Invalid Burst";
            case Scenario::reversedPolarity: return L"Reversed Polarity";
            case Scenario::faultShowcase: return L"Fault Showcase";
        }
        return L"Unknown";
    }

    [[nodiscard]] COLORREF color(const int red, const int green, const int blue) noexcept
    {
        return RGB(red, green, blue);
    }

    struct DashboardState
    {
        lsw::audio_diag::Analyzer<float> analyzer {};
        std::array<float, blockSize> left {};
        std::array<float, blockSize> right {};
        Scenario scenario = Scenario::faultShowcase;
        std::uint64_t signalSample = 0U;
        std::uint64_t scenarioBlock = 0U;
        bool paused = false;
        HFONT titleFont = nullptr;
        HFONT normalFont = nullptr;
        HFONT smallFont = nullptr;

        DashboardState() noexcept
        {
            lsw::audio_diag::AnalyzerConfig config {};
            config.sampleRate = sampleRate;
            config.maximumBlockSize = blockSize;
            config.numberOfChannels = 2U;
            config.levelTimeConstantSeconds = 0.004;
            config.dcTimeConstantSeconds = 0.004;
            config.correlationTimeConstantSeconds = 0.004;
            config.silenceHoldSeconds = 0.020;
            config.peakHoldSeconds = 1.2;
            config.peakHoldDecayDbPerSecond = 18.0;
            config.dropoutThresholdDbfs = -65.0;
            config.activeSignalThresholdDbfs = -45.0;
            config.dropoutHoldSeconds = 0.030;
            config.dropoutRecoverySeconds = 0.010;
            config.dcFaultThreshold = 0.10;
            config.dcFaultHoldSeconds = 0.030;
            config.dcFaultRecoverySeconds = 0.020;
            (void)analyzer.prepare(config);
        }

        ~DashboardState()
        {
            DeleteObject(titleFont);
            DeleteObject(normalFont);
            DeleteObject(smallFont);
        }

        void createFonts() noexcept
        {
            titleFont = CreateFontW(-28, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                                    OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                                    DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
            normalFont = CreateFontW(-17, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                                     OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                                     DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
            smallFont = CreateFontW(-14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                                    OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                                    DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
        }

        void selectScenario(const Scenario selected) noexcept
        {
            scenario = selected;
            scenarioBlock = 0U;
            signalSample = 0U;
            analyzer.reset();
        }

        void fillSine(const float amplitude, const bool invertedRight) noexcept
        {
            for (std::size_t index = 0U; index < blockSize; ++index)
            {
                const double phase = (2.0 * pi * 440.0
                                      * static_cast<double>(signalSample + index)) / sampleRate;
                left[index] = static_cast<float>(amplitude * std::sin(phase));
                right[index] = invertedRight ? -left[index] : left[index];
            }
        }

        void fillConstant(const float value) noexcept
        {
            left.fill(value);
            right.fill(value);
        }

        void fillScenarioBlock() noexcept
        {
            const std::uint64_t phase = scenarioBlock;
            switch (scenario)
            {
                case Scenario::cleanStereo:
                    fillSine(0.72F, false);
                    break;
                case Scenario::dropout:
                    if (phase < 20U)
                    {
                        fillSine(0.72F, false);
                    }
                    else if (phase < 54U)
                    {
                        fillConstant(0.0F);
                    }
                    else
                    {
                        fillSine(0.72F, false);
                    }
                    break;
                case Scenario::sustainedClip:
                    fillConstant(1.10F);
                    break;
                case Scenario::dcFault:
                    fillConstant(0.35F);
                    break;
                case Scenario::invalidBurst:
                    fillSine(0.50F, false);
                    left[0] = std::numeric_limits<float>::quiet_NaN();
                    left[1] = std::numeric_limits<float>::infinity();
                    right[0] = -std::numeric_limits<float>::infinity();
                    right[1] = std::numeric_limits<float>::quiet_NaN();
                    break;
                case Scenario::reversedPolarity:
                    fillSine(0.72F, true);
                    break;
                case Scenario::faultShowcase:
                    if (phase < 20U || phase >= 108U)
                    {
                        fillSine(0.72F, false);
                    }
                    else if (phase < 54U)
                    {
                        fillConstant(0.0F);
                    }
                    else if (phase < 64U)
                    {
                        fillConstant(1.10F);
                    }
                    else if (phase < 90U)
                    {
                        fillConstant(0.35F);
                    }
                    else if (phase < 98U)
                    {
                        fillSine(0.50F, false);
                        left[0] = std::numeric_limits<float>::quiet_NaN();
                        left[1] = std::numeric_limits<float>::infinity();
                    }
                    else
                    {
                        fillSine(0.72F, true);
                    }
                    break;
            }
        }

        void update() noexcept
        {
            if (paused)
            {
                return;
            }
            for (int block = 0; block < 8; ++block)
            {
                fillScenarioBlock();
                const float* channels[] { left.data(), right.data() };
                analyzer.process(channels, 2U, blockSize);
                signalSample += blockSize;
                ++scenarioBlock;
            }
        }
    };

    void fillRect(HDC deviceContext, const RECT& rectangle, const COLORREF fillColor) noexcept
    {
        const HBRUSH brush = CreateSolidBrush(fillColor);
        FillRect(deviceContext, &rectangle, brush);
        DeleteObject(brush);
    }

    void outlineRect(HDC deviceContext, const RECT& rectangle, const COLORREF lineColor) noexcept
    {
        const HPEN pen = CreatePen(PS_SOLID, 1, lineColor);
        const HGDIOBJ oldPen = SelectObject(deviceContext, pen);
        const HGDIOBJ oldBrush = SelectObject(deviceContext, GetStockObject(HOLLOW_BRUSH));
        Rectangle(deviceContext, rectangle.left, rectangle.top, rectangle.right, rectangle.bottom);
        SelectObject(deviceContext, oldBrush);
        SelectObject(deviceContext, oldPen);
        DeleteObject(pen);
    }

    void drawText(HDC deviceContext, const wchar_t* text, const RECT& rectangle, const HFONT font,
                  const COLORREF textColor, const UINT format = DT_LEFT | DT_VCENTER | DT_SINGLELINE) noexcept
    {
        const HGDIOBJ oldFont = SelectObject(deviceContext, font);
        SetTextColor(deviceContext, textColor);
        SetBkMode(deviceContext, TRANSPARENT);
        DrawTextW(deviceContext, text, -1, const_cast<RECT*>(&rectangle), format);
        SelectObject(deviceContext, oldFont);
    }

    void drawMetric(HDC deviceContext, const int x, const int y, const int width,
                    const wchar_t* label, const double value, const wchar_t* suffix,
                    const DashboardState& state) noexcept
    {
        RECT labelRect { x, y, x + width, y + 22 };
        drawText(deviceContext, label, labelRect, state.smallFont, color(150, 168, 184));
        wchar_t text[64] {};
        (void)swprintf_s(text, L"%.4f %ls", value, suffix);
        RECT valueRect { x, y + 19, x + width, y + 47 };
        drawText(deviceContext, text, valueRect, state.normalFont, color(230, 238, 245));
    }

    void drawMeter(HDC deviceContext, const int x, const int y, const int width, const int height,
                   const wchar_t* label, const lsw::audio_diag::ChannelMetrics& metrics,
                   const DashboardState& state) noexcept
    {
        RECT frame { x, y, x + width, y + height };
        fillRect(deviceContext, frame, color(20, 29, 38));
        outlineRect(deviceContext, frame, color(56, 77, 94));
        RECT labelRect { x + 12, y + 10, x + width - 12, y + 34 };
        drawText(deviceContext, label, labelRect, state.normalFont, color(220, 231, 240));

        const double peakDbfs = metrics.samplePeak > 1.0e-8
                                    ? 20.0 * std::log10(metrics.samplePeak) : -80.0;
        const double clampedPeak = std::max(-80.0, std::min(6.0, peakDbfs));
        const double fraction = (clampedPeak + 80.0) / 86.0;
        const int meterTop = y + 53;
        const int meterBottom = y + height - 26;
        const int meterHeight = meterBottom - meterTop;
        const int barTop = meterBottom - static_cast<int>(fraction * static_cast<double>(meterHeight));
        RECT bar { x + 22, barTop, x + width - 22, meterBottom };
        fillRect(deviceContext, bar, clampedPeak >= 0.0 ? color(226, 88, 82) : color(41, 202, 206));
        const double heldDbfs = std::max(-80.0, std::min(6.0, metrics.heldPeakDbfs));
        const int heldY = meterBottom - static_cast<int>(((heldDbfs + 80.0) / 86.0)
                                                          * static_cast<double>(meterHeight));
        const HPEN heldPen = CreatePen(PS_SOLID, 3, color(245, 178, 60));
        const HGDIOBJ oldPen = SelectObject(deviceContext, heldPen);
        MoveToEx(deviceContext, x + 14, heldY, nullptr);
        LineTo(deviceContext, x + width - 14, heldY);
        SelectObject(deviceContext, oldPen);
        DeleteObject(heldPen);

        wchar_t valueText[64] {};
        (void)swprintf_s(valueText, L"%.1f dBFS", peakDbfs);
        RECT valueRect { x + 12, y + height - 23, x + width - 12, y + height - 3 };
        drawText(deviceContext, valueText, valueRect, state.smallFont, color(159, 178, 192),
                 DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }

    void drawEventRow(HDC deviceContext, const int x, const int y, const int width,
                      const wchar_t* label, const lsw::audio_diag::EventState& event,
                      const DashboardState& state) noexcept
    {
        const COLORREF stateColor = event.active ? color(235, 86, 79)
                                                  : (event.latched ? color(246, 179, 59)
                                                                   : color(87, 111, 128));
        RECT indicator { x, y + 4, x + 10, y + 18 };
        fillRect(deviceContext, indicator, stateColor);
        RECT labelRect { x + 18, y, x + width - 160, y + 23 };
        drawText(deviceContext, label, labelRect, state.smallFont, color(218, 229, 237));
        wchar_t detail[96] {};
        (void)swprintf_s(detail, L"%ls  count %llu  longest %llu",
                         event.active ? L"ACTIVE" : (event.latched ? L"LATCHED" : L"idle"),
                         static_cast<unsigned long long>(event.eventCount),
                         static_cast<unsigned long long>(event.longestDurationSamples));
        RECT detailRect { x + width - 265, y, x + width, y + 23 };
        drawText(deviceContext, detail, detailRect, state.smallFont, stateColor,
                 DT_RIGHT | DT_VCENTER | DT_SINGLELINE);
    }

    void paintDashboard(HWND window, DashboardState& state) noexcept
    {
        PAINTSTRUCT paint {};
        HDC deviceContext = BeginPaint(window, &paint);
        RECT client {};
        GetClientRect(window, &client);
        fillRect(deviceContext, client, color(12, 18, 25));

        const lsw::audio_diag::Snapshot snapshot = state.analyzer.getSnapshot();
        RECT titleRect { 34, 22, client.right - 34, 60 };
        drawText(deviceContext, L"LSW Audio Diagnostics Core", titleRect, state.titleFont,
                 color(230, 239, 246));
        wchar_t statusText[160] {};
        (void)swprintf_s(statusText, L"v0.2.0  |  %ls  |  %ls", scenarioName(state.scenario),
                         state.paused ? L"PAUSED" : L"RUNNING");
        RECT statusRect { 36, 63, client.right - 36, 88 };
        drawText(deviceContext, statusText, statusRect, state.normalFont,
                 state.paused ? color(246, 179, 59) : color(41, 202, 206));
        for (int index = 1; index <= 7; ++index)
        {
            const int left = 34 + ((index - 1) * 118);
            RECT button { left, 91, left + 108, 108 };
            const Scenario buttonScenario = static_cast<Scenario>(index);
            fillRect(deviceContext, button, buttonScenario == state.scenario
                                                ? color(28, 92, 102) : color(29, 42, 53));
            outlineRect(deviceContext, button, color(65, 94, 111));
            wchar_t buttonText[32] {};
            (void)swprintf_s(buttonText, L"%d  %ls", index, scenarioName(buttonScenario));
            drawText(deviceContext, buttonText, button, state.smallFont, color(220, 231, 240),
                     DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        }

        drawMeter(deviceContext, 34, 112, 210, 330, L"LEFT", snapshot.channels[0], state);
        drawMeter(deviceContext, 260, 112, 210, 330, L"RIGHT", snapshot.channels[1], state);

        RECT metricPanel { 492, 112, 870, 442 };
        fillRect(deviceContext, metricPanel, color(20, 29, 38));
        outlineRect(deviceContext, metricPanel, color(56, 77, 94));
        RECT metricsHeading { 510, 122, 850, 151 };
        drawText(deviceContext, L"CHANNEL METRICS", metricsHeading, state.normalFont,
                 color(41, 202, 206));
        drawMetric(deviceContext, 512, 166, 160, L"Left Peak", snapshot.channels[0].samplePeak, L"", state);
        drawMetric(deviceContext, 684, 166, 160, L"Right Peak", snapshot.channels[1].samplePeak, L"", state);
        drawMetric(deviceContext, 512, 224, 160, L"Held Peak", snapshot.channels[0].heldPeak, L"", state);
        drawMetric(deviceContext, 684, 224, 160, L"RMS", snapshot.channels[0].rmsDbfs, L"dBFS", state);
        drawMetric(deviceContext, 512, 282, 160, L"DC Offset", snapshot.channels[0].dcOffset, L"", state);
        drawMetric(deviceContext, 684, 282, 160, L"Clip Count", static_cast<double>(snapshot.channels[0].clipCount), L"", state);
        drawMetric(deviceContext, 512, 340, 160, L"Invalid Count", static_cast<double>(snapshot.channels[0].invalidSampleCount), L"", state);
        drawMetric(deviceContext, 684, 340, 160, L"Correlation", snapshot.stereo.correlation, L"", state);

        RECT stereoPanel { 892, 112, client.right - 34, 442 };
        fillRect(deviceContext, stereoPanel, color(20, 29, 38));
        outlineRect(deviceContext, stereoPanel, color(56, 77, 94));
        RECT stereoHeading { 912, 122, client.right - 54, 151 };
        drawText(deviceContext, L"STEREO DIAGNOSTICS", stereoHeading, state.normalFont,
                 color(41, 202, 206));
        drawMetric(deviceContext, 912, 170, 180, L"Correlation", snapshot.stereo.correlation, L"", state);
        drawMetric(deviceContext, 1120, 170, 180, L"Mono Compatibility", snapshot.stereo.monoCompatibilityScore, L"", state);
        drawMetric(deviceContext, 912, 232, 180, L"Channel Balance", snapshot.stereo.channelBalanceDb, L"dB", state);
        drawMetric(deviceContext, 1120, 232, 180, L"Processed Blocks", static_cast<double>(snapshot.processedBlockCount), L"", state);
        wchar_t stereoFlags[160] {};
        (void)swprintf_s(stereoFlags, L"Identical %ls   Reversed %ls   L-only %ls   R-only %ls",
                         snapshot.stereo.identicalChannels ? L"YES" : L"no",
                         snapshot.stereo.reversedPolarity ? L"YES" : L"no",
                         snapshot.stereo.leftOnly ? L"YES" : L"no",
                         snapshot.stereo.rightOnly ? L"YES" : L"no");
        RECT flagsRect { 912, 315, client.right - 54, 350 };
        drawText(deviceContext, stereoFlags, flagsRect, state.smallFont, color(218, 229, 237));

        RECT eventPanel { 34, 466, client.right - 34, client.bottom - 86 };
        fillRect(deviceContext, eventPanel, color(20, 29, 38));
        outlineRect(deviceContext, eventPanel, color(56, 77, 94));
        RECT eventHeading { 52, 476, client.right - 54, 505 };
        drawText(deviceContext, L"DIAGNOSTIC EVENTS  (red: active, amber: latched)", eventHeading,
                 state.normalFont, color(41, 202, 206));
        drawEventRow(deviceContext, 54, 516, client.right - 108, L"Left Dropout",
                     snapshot.channelEvents[0].dropout, state);
        drawEventRow(deviceContext, 54, 542, client.right - 108, L"Left Sustained Clip",
                     snapshot.channelEvents[0].sustainedClip, state);
        drawEventRow(deviceContext, 54, 568, client.right - 108, L"Left DC Fault",
                     snapshot.channelEvents[0].dcFault, state);
        drawEventRow(deviceContext, 54, 594, client.right - 108, L"Left Invalid Burst",
                     snapshot.channelEvents[0].invalidBurst, state);
        drawEventRow(deviceContext, 54, 620, client.right - 108, L"Stereo Reversed Polarity",
                     snapshot.stereoEvents.reversedPolarity, state);

        RECT footer { 34, client.bottom - 64, client.right - 34, client.bottom - 24 };
        wchar_t footerText[256] {};
        (void)swprintf_s(footerText, L"48 kHz  |  block 128  |  samples %llu  |  1-7 scenarios  Space pause  C clear events  R reset  Esc exit",
                         static_cast<unsigned long long>(snapshot.processedSampleCount));
        drawText(deviceContext, footerText, footer, state.smallFont, color(150, 168, 184));

        EndPaint(window, &paint);
    }

    [[nodiscard]] Scenario scenarioFromPoint(const LPARAM point) noexcept
    {
        const int x = static_cast<int>(static_cast<short>(LOWORD(point)));
        const int y = static_cast<int>(static_cast<short>(HIWORD(point)));
        if (y >= 91 && y <= 108 && x >= 34 && x <= 850)
        {
            const int index = ((x - 34) / 118) + 1;
            return static_cast<Scenario>(std::max(1, std::min(7, index)));
        }
        return Scenario::faultShowcase;
    }

    LRESULT CALLBACK windowProcedure(HWND window, UINT message, WPARAM wordParameter,
                                     LPARAM longParameter) noexcept
    {
        DashboardState* state = reinterpret_cast<DashboardState*>(GetWindowLongPtrW(window, GWLP_USERDATA));
        if (message == WM_NCCREATE)
        {
            const CREATESTRUCTW* create = reinterpret_cast<const CREATESTRUCTW*>(longParameter);
            state = static_cast<DashboardState*>(create->lpCreateParams);
            SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(state));
        }
        if (state == nullptr)
        {
            return DefWindowProcW(window, message, wordParameter, longParameter);
        }

        switch (message)
        {
            case WM_GETMINMAXINFO:
            {
                auto* const info = reinterpret_cast<MINMAXINFO*>(longParameter);
                info->ptMinTrackSize.x = 1280;
                info->ptMinTrackSize.y = 720;
                return 0;
            }
            case WM_CREATE:
                state->createFonts();
                SetTimer(window, timerIdentifier, timerIntervalMilliseconds, nullptr);
                return 0;
            case WM_TIMER:
                state->update();
                InvalidateRect(window, nullptr, FALSE);
                return 0;
            case WM_PAINT:
                paintDashboard(window, *state);
                return 0;
            case WM_KEYDOWN:
                if (wordParameter >= '1' && wordParameter <= '7')
                {
                    state->selectScenario(static_cast<Scenario>(wordParameter - '0'));
                }
                else if (wordParameter == VK_SPACE)
                {
                    state->paused = !state->paused;
                }
                else if (wordParameter == 'C')
                {
                    state->analyzer.clearEvents();
                }
                else if (wordParameter == 'R')
                {
                    state->analyzer.reset();
                    state->scenarioBlock = 0U;
                    state->signalSample = 0U;
                }
                else if (wordParameter == VK_ESCAPE)
                {
                    DestroyWindow(window);
                }
                return 0;
            case WM_LBUTTONDOWN:
                state->selectScenario(scenarioFromPoint(longParameter));
                return 0;
            case WM_DESTROY:
                KillTimer(window, timerIdentifier);
                PostQuitMessage(0);
                return 0;
            default:
                return DefWindowProcW(window, message, wordParameter, longParameter);
        }
    }
}

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int showCommand)
{
    DashboardState state {};
    const wchar_t className[] = L"LSWAudioDiagnosticsDashboardWindow";
    WNDCLASSW windowClass {};
    windowClass.hInstance = instance;
    windowClass.lpszClassName = className;
    windowClass.lpfnWndProc = windowProcedure;
    windowClass.hCursor = LoadCursorW(nullptr, MAKEINTRESOURCEW(32512));
    windowClass.hbrBackground = static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
    if (RegisterClassW(&windowClass) == 0U)
    {
        return 1;
    }

    HWND window = CreateWindowExW(0U, className, L"LSW Audio Diagnostics Dashboard v0.2.0",
                                  WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
                                  1600, 900, nullptr, nullptr, instance, &state);
    if (window == nullptr)
    {
        return 1;
    }
    ShowWindow(window, showCommand);
    UpdateWindow(window);

    MSG message {};
    while (GetMessageW(&message, nullptr, 0U, 0U) > 0)
    {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }
    return static_cast<int>(message.wParam);
}
