// Generated C++ file by Il2CppInspector - http://www.djkaty.com - https://github.com/djkaty
// Helper functions

#include "pch-il2cpp.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string>
#include <codecvt>
#include "helpers.h"


// Helper function to get the module base address
uintptr_t il2cppi_get_base_address() {
    return (uintptr_t) GetModuleHandleW(L"UserAssembly.dll");
}

uintptr_t il2cppi_get_unity_address() {
    return (uintptr_t)GetModuleHandleW(L"UnityPlayer.dll");
}

// Helper function to open a new console window and redirect stdout there
void il2cppi_new_console() {
    AllocConsole();
    freopen_s((FILE**) stdout, "CONOUT$", "w", stdout);
    freopen_s((FILE**) stderr, "CONOUT$", "w", stderr);
}

void il2cppi_close_console() {
    fclose(stdout);
    fclose(stderr);  
    FreeConsole();
}

#if _MSC_VER >= 1920
// Helper function to convert Il2CppString to std::string
std::string il2cppi_to_string(Il2CppString* str) {
    std::u16string u16(reinterpret_cast<const char16_t*>(str->chars));
    return std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t>{}.to_bytes(u16);
}

std::string il2cppi_to_string(app::Vector vec) {
    return string_format("{ x = %f, y = %f, z = %f }", vec._x, vec._y, vec._z);
}

std::string il2cppi_to_string(app::Vector2 vec) {
    return string_format("{ x = %f, y = %f }", vec.x, vec.y);
}

std::string il2cppi_to_string(app::Vector3 vec) {
    return string_format("{ x = %f, y = %f, z = %f }", vec.x, vec.y, vec.z);
}

// Helper function to convert System.String to std::string
std::string il2cppi_to_string(app::String* str) {
    if (str == nullptr)
        return std::string("<nullptr>");
    return il2cppi_to_string(reinterpret_cast<Il2CppString*>(str));
}
#endif