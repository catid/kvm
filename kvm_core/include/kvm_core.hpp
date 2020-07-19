// Copyright 2020 Christopher A. Taylor

/*
    Core tools used by multiple KVM sub-projects
*/

#include <cstdint>      // uint8_t etc types
#include <cstring>
#include <memory>       // std::unique_ptr, std::shared_ptr
#include <new>          // std::nothrow
#include <mutex>        // std::mutex
#include <vector>       // std::vector
#include <functional>   // std::function
#include <thread>       // std::thread
#include <string>

namespace kvm {


//------------------------------------------------------------------------------
// Constants

static const int kAppSuccess = 0;
static const int kAppFail = -1;


//------------------------------------------------------------------------------
// Portability Macros

#define CORE_ARRAY_COUNT(array) \
    static_cast<int>(sizeof(array) / sizeof(array[0]))

// Specify an intentionally unused variable (often a function parameter)
#define CORE_UNUSED(x) (void)(x);

// Compiler-specific debug break
#if defined(_DEBUG) || defined(DEBUG) || defined(CORE_DEBUG_IN_RELEASE)
    #define CORE_DEBUG
    #if defined(_WIN32)
        #define CORE_DEBUG_BREAK() __debugbreak()
    #else // _WIN32
        #define CORE_DEBUG_BREAK() __builtin_trap()
    #endif // _WIN32
    #define CORE_DEBUG_ASSERT(cond) { if (!(cond)) { CORE_DEBUG_BREAK(); } }
    #define CORE_IF_DEBUG(x) x;
#else // _DEBUG
    #define CORE_DEBUG_BREAK() ;
    #define CORE_DEBUG_ASSERT(cond) ;
    #define CORE_IF_DEBUG(x) ;
#endif // _DEBUG

// Compiler-specific force inline keyword
#if defined(_MSC_VER)
    #define CORE_INLINE inline __forceinline
#else // _MSC_VER
    #define CORE_INLINE inline __attribute__((always_inline))
#endif // _MSC_VER

// Compiler-specific align keyword
#if defined(_MSC_VER)
    #define CORE_ALIGNED(x) __declspec(align(x))
#else // _MSC_VER
    #define CORE_ALIGNED(x) __attribute__ ((aligned(x)))
#endif // _MSC_VER


//------------------------------------------------------------------------------
// C++ Convenience Classes

/// Calls the provided (lambda) function at the end of the current scope
class ScopedFunction
{
public:
    ScopedFunction(std::function<void()> func) {
        Func = func;
    }
    ~ScopedFunction() {
        Func();
    }

    std::function<void()> Func;
};

/// Join a std::shared_ptr<std::thread>
inline void JoinThread(std::shared_ptr<std::thread>& th)
{
    if (th) {
        try {
            if (th->joinable()) {
                th->join();
            }
        } catch (std::system_error& /*err*/) {}
        th = nullptr;
    }
}


//------------------------------------------------------------------------------
// High-resolution timers

/// Get time in microseconds
uint64_t GetTimeUsec();

/// Get time in milliseconds
uint64_t GetTimeMsec();


//------------------------------------------------------------------------------
// Thread Tools

/// Set the current thread name
void SetCurrentThreadName(const char* name);


//------------------------------------------------------------------------------
// String Conversion

/// Convert buffer to hex string
std::string HexDump(const uint8_t* data, int bytes);

/// Convert value to hex string
std::string HexString(uint64_t value);


//------------------------------------------------------------------------------
// Copy Strings

inline void SafeCopyCStr(char* dest, size_t destBytes, const char* src)
{
#if defined(_MSC_VER)
    ::strncpy_s(dest, destBytes, src, _TRUNCATE);
#else // _MSC_VER
    ::strncpy(dest, src, destBytes);
#endif // _MSC_VER
    // No null-character is implicitly appended at the end of destination
    dest[destBytes - 1] = '\0';
}


} // namespace kvm
