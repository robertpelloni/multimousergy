#include "ClipboardModule.hpp"
#include <iostream>
#include <vector>

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif

ClipboardModule::ClipboardModule() : m_lastHash(0) {}
ClipboardModule::~ClipboardModule() {}

bool ClipboardModule::HasChanged() {
#ifdef _WIN32
    if (!OpenClipboard(NULL)) return false;

    // Use CF_UNICODETEXT for universal support
    HANDLE hData = GetClipboardData(CF_UNICODETEXT);
    if (hData == NULL) {
        CloseClipboard();
        return false;
    }

    wchar_t* pWText = static_cast<wchar_t*>(GlobalLock(hData));
    std::wstring wCurrentText = pWText ? pWText : L"";
    GlobalUnlock(hData);
    CloseClipboard();

    std::string currentText = Utf16ToUtf8(wCurrentText);
    size_t currentHash = std::hash<std::string>{}(currentText);

    if (currentHash != m_lastHash) {
        m_lastHash = currentHash;
        m_lastText = currentText;
        return true;
    }
#endif
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
    m_lastText = text;
    m_lastHash = currentHash;
    std::cout << "[Clipboard] Applied remote update (Unicode)." << std::endl;
#endif
}

std::string ClipboardModule::Utf16ToUtf8(const std::wstring& wstr) {
    if (wstr.empty()) return "";
#ifdef _WIN32
    int sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string strTo(sizeNeeded, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.size(), &strTo[0], sizeNeeded, NULL, NULL);
    return strTo;
#else
    return ""; // TODO: Linux/macOS iconv
#endif
}

std::wstring ClipboardModule::Utf8ToUtf16(const std::string& str) {
    if (str.empty()) return L"";
#ifdef _WIN32
    int sizeNeeded = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), NULL, 0);
    std::wstring wstrTo(sizeNeeded, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), &wstrTo[0], sizeNeeded);
    return wstrTo;
#else
    return L""; // TODO: Linux/macOS iconv
#endif
}
