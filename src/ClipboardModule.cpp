#include "ClipboardModule.hpp"
#include <iostream>

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif

ClipboardModule::ClipboardModule() {}
ClipboardModule::~ClipboardModule() {}

bool ClipboardModule::HasChanged() {
#ifdef _WIN32
    if (!OpenClipboard(NULL)) return false;

    HANDLE hData = GetClipboardData(CF_TEXT);
    if (hData == NULL) {
        CloseClipboard();
        return false;
    }

    char* pszText = static_cast<char*>(GlobalLock(hData));
    std::string currentText = pszText ? pszText : "";
    GlobalUnlock(hData);
    CloseClipboard();

    if (currentText != m_lastText) {
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
    if (text == m_lastText) return;

#ifdef _WIN32
    if (!OpenClipboard(NULL)) return;
    EmptyClipboard();

    HGLOBAL hGlob = GlobalAlloc(GMEM_MOVEABLE, text.size() + 1);
    if (hGlob) {
        memcpy(GlobalLock(hGlob), text.c_str(), text.size() + 1);
        GlobalUnlock(hGlob);
        SetClipboardData(CF_TEXT, hGlob);
    }
    CloseClipboard();
    m_lastText = text;
    std::cout << "[Clipboard] Applied remote update." << std::endl;
#endif
}
