#include "ClipboardModule.hpp"
#include <iostream>
#include <vector>
#include <codecvt>
#include <locale>

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif

#ifdef __linux__
#include <stdio.h>
#include <memory>
#include <array>
#endif

ClipboardModule::ClipboardModule() : m_lastHash(0) {}
ClipboardModule::~ClipboardModule() {}

bool ClipboardModule::HasChanged() {
    std::string currentText;
#ifdef _WIN32
    if (!OpenClipboard(NULL)) return false;

    HANDLE hData = GetClipboardData(CF_UNICODETEXT);
    if (hData == NULL) {
        CloseClipboard();
        return false;
    }

    wchar_t* pWText = static_cast<wchar_t*>(GlobalLock(hData));
    std::wstring wCurrentText = pWText ? pWText : L"";
    GlobalUnlock(hData);
    CloseClipboard();

    currentText = Utf16ToUtf8(wCurrentText);
#elif defined(__linux__)
    // Basic Linux stub using xclip
    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen("xclip -o -selection clipboard 2>/dev/null", "r"), pclose);
    if (pipe) {
        while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
            result += buffer.data();
        }
    }
    currentText = result;
#endif

    if (currentText.empty() && m_lastText.empty()) return false;

    size_t currentHash = std::hash<std::string>{}(currentText);
    if (currentHash != m_lastHash) {
        m_lastHash = currentHash;
        m_lastText = currentText;
        return true;
    }
    return false;
}

std::string ClipboardModule::GetText() {
    return m_lastText;
}

void ClipboardModule::SetText(const std::string& text) {
    size_t currentHash = std::hash<std::string>{}(text);
    if (currentHash == m_lastHash) return;

#ifdef _WIN32
    if (!OpenClipboard(NULL)) return;
    EmptyClipboard();

    std::wstring wText = Utf8ToUtf16(text);
    size_t sizeInBytes = (wText.size() + 1) * sizeof(wchar_t);

    HGLOBAL hGlob = GlobalAlloc(GMEM_MOVEABLE, sizeInBytes);
    if (hGlob) {
        memcpy(GlobalLock(hGlob), wText.c_str(), sizeInBytes);
        GlobalUnlock(hGlob);
        SetClipboardData(CF_UNICODETEXT, hGlob);
    }
    CloseClipboard();
#elif defined(__linux__)
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen("xclip -selection clipboard", "w"), pclose);
    if (pipe) {
        fwrite(text.c_str(), 1, text.size(), pipe.get());
    }
#endif

    m_lastText = text;
    m_lastHash = currentHash;
    std::cout << "[Clipboard] Applied remote update." << std::endl;
}

std::string ClipboardModule::Utf16ToUtf8(const std::wstring& wstr) {
    if (wstr.empty()) return "";
    try {
        std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
        return converter.to_bytes(wstr);
    } catch (...) {
        return "";
    }
}

std::wstring ClipboardModule::Utf8ToUtf16(const std::string& str) {
    if (str.empty()) return L"";
    try {
        std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
        return converter.from_bytes(str);
    } catch (...) {
        return L"";
    }
}
