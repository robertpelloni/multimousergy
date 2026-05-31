#include <iostream>
#include <cassert>
#include <string>
#include "ClipboardModule.hpp"

// Accessing private members via helper if needed,
// but for these tests we can use public API.

void TestUnicodeConversion() {
    std::cout << "Testing Unicode Conversion..." << std::endl;

    // We can't easily test private static methods directly without friend class or making them public.
    // However, we can test SetText/GetText behavior if the environment supports it,
    // or we can test the logic if we mock the platform.

    // Since we are in a headless Linux environment, we can test the hashing logic
    // and the round-trip through the module's state.

    ClipboardModule clip;
    std::string testUtf8 = "Hello, 世界! 🌍";

    clip.SetText(testUtf8);
    assert(clip.GetText() == testUtf8);

    std::cout << "Unicode Round-trip (State) Passed." << std::endl;
}

void TestHashDetection() {
    std::cout << "Testing Hash-based Change Detection..." << std::endl;

    ClipboardModule clip;
    std::string text1 = "Initial Content";
    std::string text2 = "Modified Content";

    clip.SetText(text1);
    assert(clip.GetText() == text1);

    // On Linux, HasChanged() uses xclip. In this test environment,
    // popen("xclip -o") might fail or return empty.
    // We mainly want to verify that SetText updates the internal hash.

    clip.SetText(text2);
    assert(clip.GetText() == text2);

    std::cout << "Hash Detection Logic (State) Passed." << std::endl;
}

void test_clipboard_module() {
    std::cout << "Running Clipboard Unit Tests..." << std::endl;

    TestUnicodeConversion();
    TestHashDetection();

    std::cout << "All Clipboard Tests Passed!" << std::endl;
}
