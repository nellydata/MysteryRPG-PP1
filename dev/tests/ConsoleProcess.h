#pragma once

// QA process transport only. The game remains independent of Windows APIs.
#ifdef _WIN32
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#else
#include <cerrno>
#include <csignal>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#endif
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
#ifdef _WIN32
    HANDLE process_ = nullptr, input_ = nullptr, output_ = nullptr;
    static void Close(HANDLE& handle) { if (handle) { CloseHandle(handle); handle = nullptr; } }
#else
    pid_t process_ = -1;
    int input_ = -1, output_ = -1;
    mutable int exitCode_ = -1;
    static void Close(int& handle) { if (handle >= 0) { close(handle); handle = -1; } }
    void RefreshExit() const {
        if (process_ <= 0 || exitCode_ >= 0) return;
        int status = 0;
        const pid_t result = waitpid(process_, &status, WNOHANG);
        if (result == process_)
            exitCode_ = WIFEXITED(status) ? WEXITSTATUS(status) : 128 + WTERMSIG(status);
    }
#endif
public:
    Process(const fs::path& executable, const std::vector<std::wstring>& arguments,
            const fs::path& workingDirectory) {
#ifdef _WIN32
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
#else
        int childInput[2]{-1, -1}, childOutput[2]{-1, -1};
        if (pipe(childInput) || pipe(childOutput)) throw std::runtime_error("pipe failed");
        process_ = fork();
        if (process_ < 0) throw std::runtime_error("fork failed");
        if (process_ == 0) {
            dup2(childInput[0], STDIN_FILENO);
            dup2(childOutput[1], STDOUT_FILENO);
            dup2(childOutput[1], STDERR_FILENO);
            close(childInput[0]); close(childInput[1]); close(childOutput[0]); close(childOutput[1]);
            if (chdir(workingDirectory.c_str()) != 0) _exit(126);
            std::vector<std::string> storage{executable.string()};
            for (const auto& argument : arguments) storage.emplace_back(argument.begin(), argument.end());
            std::vector<char*> argv;
            for (auto& argument : storage) argv.push_back(argument.data());
            argv.push_back(nullptr);
            execv(executable.c_str(), argv.data());
            _exit(127);
        }
        close(childInput[0]); close(childOutput[1]);
        input_ = childInput[1]; output_ = childOutput[0];
        fcntl(output_, F_SETFL, fcntl(output_, F_GETFL) | O_NONBLOCK);
#endif
    }
    Process(const Process&) = delete;
    Process& operator=(const Process&) = delete;
    ~Process() {
        CloseInput();
#ifdef _WIN32
        if (process_ && WaitForSingleObject(process_, 100) == WAIT_TIMEOUT) {
            // Only the child created by this QA object is eligible for cleanup.
            TerminateProcess(process_, 1); WaitForSingleObject(process_, 1000);
        }
        Close(process_); Close(output_);
#else
        RefreshExit();
        if (!Exited() && process_ > 0) { kill(process_, SIGTERM); waitpid(process_, nullptr, 0); }
        Close(output_);
#endif
    }
    void CloseInput() { Close(input_); }
#ifdef _WIN32
    bool Exited() const { return WaitForSingleObject(process_, 0) == WAIT_OBJECT_0; }
    bool WaitForExit(unsigned milliseconds = 5000) const { return WaitForSingleObject(process_, milliseconds) == WAIT_OBJECT_0; }
    unsigned ExitCode() const { DWORD code = STILL_ACTIVE; GetExitCodeProcess(process_, &code); return code; }
#else
    bool Exited() const { RefreshExit(); return exitCode_ >= 0; }
    bool WaitForExit(unsigned milliseconds = 5000) const {
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(milliseconds);
        while (!Exited() && std::chrono::steady_clock::now() < deadline)
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        return Exited();
    }
    unsigned ExitCode() const { RefreshExit(); return exitCode_ < 0 ? 259U : static_cast<unsigned>(exitCode_); }
#endif
    void Send(const std::string& line) {
        const std::string bytes = line + "\n";
#ifdef _WIN32
        DWORD written = 0;
        if (!input_ || !::WriteFile(input_, bytes.data(), static_cast<DWORD>(bytes.size()), &written, nullptr) || written != bytes.size())
            throw std::runtime_error("Console input pipe closed before the next key");
#else
        const auto written = input_ < 0 ? -1 : write(input_, bytes.data(), bytes.size());
        if (written != static_cast<ssize_t>(bytes.size()))
            throw std::runtime_error("Console input pipe closed before the next key");
#endif
    }
    std::string Read(bool prompt, std::ostream* capture = nullptr, unsigned timeoutSeconds = 30) {
        std::string text;
        auto lastOutput = std::chrono::steady_clock::now();
        for (;;) {
#ifdef _WIN32
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
#else
            char buffer[8192];
            const ssize_t count = read(output_, buffer, sizeof(buffer));
            if (count > 0) {
                text.append(buffer, static_cast<std::size_t>(count));
                if (capture) { capture->write(buffer, count); capture->flush(); }
                lastOutput = std::chrono::steady_clock::now();
                if (prompt && text.size() >= 2 && text.compare(text.size() - 2, 2, "> ") == 0) break;
                continue;
            }
            if (count == 0) break;
            if (errno != EAGAIN && errno != EWOULDBLOCK) break;
#endif
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
#ifdef _WIN32
    const auto bytes = ReadFile(executable);
    if (bytes.size() < 0x40) throw std::runtime_error("Executable has no PE header");
    unsigned offset = 0;
    for (unsigned i = 0; i < 4; ++i) offset |= static_cast<unsigned char>(bytes[0x3c + i]) << (8 * i);
    if (offset + 94 > bytes.size() || bytes.substr(offset, 4) != std::string("PE\0\0", 4) ||
        static_cast<unsigned char>(bytes[offset + 24 + 68]) != 3 || bytes[offset + 24 + 69] != 0)
        throw std::runtime_error("Expected PE console subsystem 3");
#else
    if (!fs::exists(executable) || access(executable.c_str(), X_OK) != 0)
        throw std::runtime_error("Expected an executable console application");
#endif
}
}
