#include "ClipboardModule.hpp"
#include <iostream>
#include <vector>

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif

#ifdef __linux__
#include <stdio.h>
#include <memory>
#include <array>
#include <thread>
#include <chrono>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#endif

#ifdef __linux__
ClipboardModule::ClipboardModule(void* xDisplay) : m_xDisplay(xDisplay), m_lastHash(0) {}
#else
ClipboardModule::ClipboardModule() : m_lastHash(0) {}
#endif
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
    Display* display = (Display*)m_xDisplay;
    if (display) {
        // We use a temporary window for conversion, or we could use the framework's window.
        // For reading, a temporary one is fine to avoid interfering with framework events.
        Window window = XCreateSimpleWindow(display, DefaultRootWindow(display), 0, 0, 1, 1, 0, 0, 0);
        Atom clipboard = XInternAtom(display, "CLIPBOARD", False);
        Atom target = XInternAtom(display, "UTF8_STRING", False);
        Atom property = XInternAtom(display, "XSEL_DATA", False);

        XConvertSelection(display, clipboard, target, property, window, CurrentTime);
        XFlush(display);

        XEvent event;
        bool ready = false;
        for (int i = 0; i < 100; ++i) { // Simple timeout loop
            if (XCheckTypedEvent(display, SelectionNotify, &event)) {
                ready = true;
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }

        if (ready) {
            Atom actualType;
            int actualFormat;
            unsigned long nitems, bytesAfter;
            unsigned char* data = nullptr;
            if (XGetWindowProperty(display, window, property, 0, 1024 * 1024, False, AnyPropertyType,
                                   &actualType, &actualFormat, &nitems, &bytesAfter, &data) == Success && data) {
                currentText = (char*)data;
                XFree(data);
            }
        }
        XDestroyWindow(display, window);
    }

    if (currentText.empty()) {
        // Fallback to xclip if X11 failed or returned empty
        std::array<char, 128> buffer;
        std::unique_ptr<FILE, decltype(&pclose)> pipe(popen("xclip -o -selection clipboard 2>/dev/null", "r"), pclose);
        if (pipe) {
            while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
                currentText += buffer.data();
            }
        }
    }
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
    Display* display = (Display*)m_xDisplay;
    if (display) {
        Atom clipboard = XInternAtom(display, "CLIPBOARD", False);
        // We use our existing framework window to take ownership
        // But since ClipboardModule doesn't know about it, we'll use a hidden root-owned window
        // For production, we should pass the framework window handle here.
        // For now, let's use the default root window or a simple invisible one.
        Window window = XCreateSimpleWindow(display, DefaultRootWindow(display), 0, 0, 1, 1, 0, 0, 0);
        XSetSelectionOwner(display, clipboard, window, CurrentTime);
        XFlush(display);
        // NOTE: The actual data serving happens in NetMuxFramework::ProcessX11Events
    }

    // Keep xclip as a reliable fallback for now
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen("xclip -selection clipboard", "w"), pclose);
    if (pipe) {
        fwrite(text.c_str(), 1, text.size(), pipe.get());
    }
#endif

    m_lastText = text;
    m_lastHash = currentHash;
    std::cout << "[Clipboard] Applied remote update." << std::endl;
}

#ifdef __linux__
void ClipboardModule::HandleX11Event(void* evPtr) {
    XEvent* ev = (XEvent*)evPtr;
    if (ev->type == SelectionRequest) {
        Display* display = (Display*)m_xDisplay;
        XSelectionRequestEvent* req = &ev->xselectionrequest;
        XSelectionEvent reply = {0};
        reply.type = SelectionNotify;
        reply.display = display;
        reply.requestor = req->requestor;
        reply.selection = req->selection;
        reply.target = req->target;
        reply.property = req->property;
        reply.time = req->time;

        Atom utf8 = XInternAtom(display, "UTF8_STRING", False);
        if (req->target == utf8 || req->target == XA_STRING) {
            XChangeProperty(display, req->requestor, req->property, req->target, 8, PropModeReplace,
                            (unsigned char*)m_lastText.c_str(), m_lastText.size());
        } else {
            reply.property = None;
        }
        XSendEvent(display, req->requestor, True, 0, (XEvent*)&reply);
        XFlush(display);
    }
}
#endif

std::string ClipboardModule::Utf16ToUtf8(const std::wstring& wstr) {
    if (wstr.empty()) return "";
    std::string out;
    for (wchar_t wc : wstr) {
        if (wc < 0x80) {
            out += (char)wc;
        } else if (wc < 0x800) {
            out += (char)(0xC0 | (wc >> 6));
            out += (char)(0x80 | (wc & 0x3F));
        } else {
            out += (char)(0xE0 | (wc >> 12));
            out += (char)(0x80 | ((wc >> 6) & 0x3F));
            out += (char)(0x80 | (wc & 0x3F));
        }
    }
    return out;
}

void ClipboardModule::CleanupPeer(unsigned long long id) {
    // No explicit state stored per-peer in ClipboardModule yet,
    // but this serves as a hook for future multi-peer clipboard isolation.
}

std::wstring ClipboardModule::Utf8ToUtf16(const std::string& str) {
    if (str.empty()) return L"";
    std::wstring out;
    for (size_t i = 0; i < str.size(); ++i) {
        unsigned char c = (unsigned char)str[i];
        if (c < 0x80) {
            out += (wchar_t)c;
        } else if ((c & 0xE0) == 0xC0) {
            wchar_t wc = (c & 0x1F) << 6;
            wc |= (str[++i] & 0x3F);
            out += wc;
        } else if ((c & 0xF0) == 0xE0) {
            wchar_t wc = (c & 0x0F) << 12;
            wc |= (str[++i] & 0x3F) << 6;
            wc |= (str[++i] & 0x3F);
            out += wc;
        }
    }
    return out;
}
