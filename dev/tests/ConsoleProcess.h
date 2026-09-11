#pragma once

// QA process transport only. The game remains independent of Windows APIs.
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

namespace ConsoleQA {
namespace fs = std::filesystem;

inline std::wstring Quote(const std::wstring& value) {
    std::wstring result = L"\"";
    unsigned slashes = 0;
    for (wchar_t c : value) {
        if (c == L'\\') { ++slashes; continue; }
        if (c == L'"') result.append(slashes * 2 + 1, L'\\');
        else result.append(slashes, L'\\');
        slashes = 0; result += c;
    }
    result.append(slashes * 2, L'\\');
    return result + L'"';
}

inline std::string ReadFile(const fs::path& path) {
    std::ifstream stream(path, std::ios::binary);
    if (!stream) throw std::runtime_error("Cannot read " + path.string());
    return {std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>()};
}

inline void WriteFile(const fs::path& path, const std::string& text) {
    std::ofstream stream(path, std::ios::binary);
    if (!stream) throw std::runtime_error("Cannot write " + path.string());
    stream << text;
}

class Process {
    HANDLE process_ = nullptr, input_ = nullptr, output_ = nullptr;
    static void Close(HANDLE& handle) { if (handle) { CloseHandle(handle); handle = nullptr; } }
public:
    Process(const fs::path& executable, const std::vector<std::wstring>& arguments,
            const fs::path& workingDirectory) {
        SECURITY_ATTRIBUTES security{sizeof(SECURITY_ATTRIBUTES), nullptr, TRUE};
        HANDLE childInput = nullptr, childOutput = nullptr;
        if (!CreatePipe(&childInput, &input_, &security, 0) ||
            !CreatePipe(&output_, &childOutput, &security, 0)) {
            Close(childInput); Close(childOutput); Close(input_); Close(output_);
            throw std::runtime_error("CreatePipe failed");
        }
        SetHandleInformation(input_, HANDLE_FLAG_INHERIT, 0);
        SetHandleInformation(output_, HANDLE_FLAG_INHERIT, 0);
        STARTUPINFOW startup{};
        startup.cb = sizeof(startup); startup.dwFlags = STARTF_USESTDHANDLES;
        startup.hStdInput = childInput; startup.hStdOutput = childOutput; startup.hStdError = childOutput;
        PROCESS_INFORMATION information{};
        std::wstring command = Quote(executable.wstring());
        for (const auto& argument : arguments) command += L" " + Quote(argument);
        const BOOL created = CreateProcessW(executable.c_str(), command.data(), nullptr, nullptr,
            TRUE, CREATE_NO_WINDOW, nullptr, workingDirectory.c_str(), &startup, &information);
        Close(childInput); Close(childOutput);
        if (!created) { Close(input_); Close(output_); throw std::runtime_error("CreateProcess failed: " + std::to_string(GetLastError())); }
        process_ = information.hProcess; CloseHandle(information.hThread);
    }
    Process(const Process&) = delete;
    Process& operator=(const Process&) = delete;
    ~Process() {
        CloseInput();
        if (process_ && WaitForSingleObject(process_, 100) == WAIT_TIMEOUT) {
            // Only the child created by this QA object is eligible for cleanup.
            TerminateProcess(process_, 1); WaitForSingleObject(process_, 1000);
        }
        Close(process_); Close(output_);
    }
    void CloseInput() { Close(input_); }
    bool Exited() const { return WaitForSingleObject(process_, 0) == WAIT_OBJECT_0; }
    bool WaitForExit(unsigned milliseconds = 5000) const { return WaitForSingleObject(process_, milliseconds) == WAIT_OBJECT_0; }
    unsigned ExitCode() const { DWORD code = STILL_ACTIVE; GetExitCodeProcess(process_, &code); return code; }
    void Send(const std::string& line) {
        const std::string bytes = line + "\n";
        DWORD written = 0;
        if (!input_ || !::WriteFile(input_, bytes.data(), static_cast<DWORD>(bytes.size()), &written, nullptr) || written != bytes.size())
            throw std::runtime_error("Console input pipe closed before the next key");
    }
    std::string Read(bool prompt, std::ostream* capture = nullptr, unsigned timeoutSeconds = 30) {
        std::string text;
        auto lastOutput = std::chrono::steady_clock::now();
        for (;;) {
            DWORD available = 0;
            if (!PeekNamedPipe(output_, nullptr, 0, nullptr, &available, nullptr)) break;
            if (available) {
                char buffer[8192]; DWORD count = 0;
                if (!::ReadFile(output_, buffer, (std::min)(available, static_cast<DWORD>(sizeof(buffer))), &count, nullptr) || !count) break;
                text.append(buffer, count);
                if (capture) { capture->write(buffer, count); capture->flush(); }
                lastOutput = std::chrono::steady_clock::now();
                if (prompt && text.size() >= 3 && text.compare(text.size() - 3, 3, "\n> ") == 0) break;
                continue;
            }
            if (Exited()) break;
            if (std::chrono::steady_clock::now() - lastOutput > std::chrono::seconds(timeoutSeconds))
                throw std::runtime_error("QA transport timeout waiting for console output (not proof of a game failure)");
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        return text;
    }
};

inline std::string CaptureProcess(const fs::path& executable, const std::vector<std::wstring>& args,
                                  const fs::path& cwd, unsigned& exitCode) {
    Process child(executable, args, cwd); child.CloseInput();
    auto output = child.Read(false, nullptr, 180);
        if (!child.WaitForExit()) throw std::runtime_error("Child closed output but did not terminate within five seconds");
    exitCode = child.ExitCode(); return output;
}

inline void RequireConsole(const fs::path& executable) {
    const auto bytes = ReadFile(executable);
    if (bytes.size() < 0x40) throw std::runtime_error("Executable has no PE header");
    unsigned offset = 0;
    for (unsigned i = 0; i < 4; ++i) offset |= static_cast<unsigned char>(bytes[0x3c + i]) << (8 * i);
    if (offset + 94 > bytes.size() || bytes.substr(offset, 4) != std::string("PE\0\0", 4) ||
        static_cast<unsigned char>(bytes[offset + 24 + 68]) != 3 || bytes[offset + 24 + 69] != 0)
        throw std::runtime_error("Expected PE console subsystem 3");
}
}
