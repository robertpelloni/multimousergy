#pragma once
#include <string>

class ClipboardModule {
public:
#ifdef __linux__
    ClipboardModule(void* xDisplay = nullptr);
#else
    ClipboardModule();
#endif
    ~ClipboardModule();

    bool HasChanged();
    std::string GetText();
    void SetText(const std::string& text);

private:
#ifdef __linux__
    void* m_xDisplay = nullptr;
#endif
    std::string m_lastText;
    size_t m_lastHash = 0;

    static std::string Utf16ToUtf8(const std::wstring& wstr);
    static std::wstring Utf8ToUtf16(const std::string& str);
};
