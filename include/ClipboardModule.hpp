#pragma once
#include <string>

class ClipboardModule {
public:
    ClipboardModule();
    ~ClipboardModule();

    bool HasChanged();
    std::string GetText();
    void SetText(const std::string& text);

private:
    std::string m_lastText;
};
