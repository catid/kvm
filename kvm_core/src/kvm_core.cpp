// Copyright 2020 Christopher A. Taylor

#include "kvm_core.hpp"
#include "kvm_serializer.hpp"

#if !defined(_WIN32)
    #include <pthread.h>
    #include <unistd.h>
#endif // _WIN32

#ifdef _WIN32
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
    #include <windows.h>
#elif __MACH__
    #include <sys/file.h>
    #include <mach/mach_time.h>
    #include <mach/mach.h>
    #include <mach/clock.h>

    extern mach_port_t clock_port;
#else
    #include <time.h>
    #include <sys/time.h>
    #include <sys/file.h> // flock
#endif

#include <fstream> // ofstream
#include <iomanip> // setw, setfill
#include <cctype>
#include <sstream>

namespace kvm {


//------------------------------------------------------------------------------
// Timing

#ifdef _WIN32
// Precomputed frequency inverse
static double PerfFrequencyInverseUsec = 0.;
static double PerfFrequencyInverseMsec = 0.;

static void InitPerfFrequencyInverse()
{
    LARGE_INTEGER freq = {};
    if (!::QueryPerformanceFrequency(&freq) || freq.QuadPart == 0) {
        return;
    }
    const double invFreq = 1. / (double)freq.QuadPart;
    PerfFrequencyInverseUsec = 1000000. * invFreq;
    PerfFrequencyInverseMsec = 1000. * invFreq;
    CORE_DEBUG_ASSERT(PerfFrequencyInverseUsec > 0.);
    CORE_DEBUG_ASSERT(PerfFrequencyInverseMsec > 0.);
}
#elif __MACH__
static bool m_clock_serv_init = false;
static clock_serv_t m_clock_serv = 0;

static void InitClockServ()
{
    m_clock_serv_init = true;
    host_get_clock_service(mach_host_self(), SYSTEM_CLOCK, &m_clock_serv);
}
#endif // _WIN32

uint64_t GetTimeUsec()
{
#ifdef _WIN32
    LARGE_INTEGER timeStamp = {};
    if (!::QueryPerformanceCounter(&timeStamp)) {
        return 0;
    }
    if (PerfFrequencyInverseUsec == 0.) {
        InitPerfFrequencyInverse();
    }
    return (uint64_t)(PerfFrequencyInverseUsec * timeStamp.QuadPart);
#elif __MACH__
    if (!m_clock_serv_init) {
        InitClockServ();
    }

    mach_timespec_t tv;
    clock_get_time(m_clock_serv, &tv);

    return 1000000 * tv.tv_sec + tv.tv_nsec / 1000;
#else
    // This seems to be the best clock to used based on:
    // http://btorpey.github.io/blog/2014/02/18/clock-sources-in-linux/
    // The CLOCK_MONOTONIC_RAW seems to take a long time to query,
    // and so it only seems useful for timing code that runs a small number of times.
    // The CLOCK_MONOTONIC is affected by NTP at 500ppm but doesn't make sudden jumps.
    // Applications should already be robust to clock skew so this is acceptable.
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return static_cast<uint64_t>(ts.tv_nsec / 1000) + static_cast<uint64_t>(ts.tv_sec) * 1000000;
#endif
}

uint64_t GetTimeMsec()
{
#ifdef _WIN32
    LARGE_INTEGER timeStamp = {};
    if (!::QueryPerformanceCounter(&timeStamp)) {
        return 0;
    }
    if (PerfFrequencyInverseMsec == 0.) {
        InitPerfFrequencyInverse();
    }
    return (uint64_t)(PerfFrequencyInverseMsec * timeStamp.QuadPart);
#else
    // TBD: Optimize this?
    return GetTimeUsec() / 1000;
#endif
}


//------------------------------------------------------------------------------
// Thread Tools

#ifdef _WIN32
const DWORD MS_VC_EXCEPTION = 0x406D1388;
#pragma pack(push,8)
typedef struct tagTHREADNAME_INFO
{
    DWORD dwType;       // Must be 0x1000.
    LPCSTR szName;      // Pointer to name (in user addr space).
    DWORD dwThreadID;   // Thread ID (-1=caller thread).
    DWORD dwFlags;      // Reserved for future use, must be zero.
} THREADNAME_INFO;
#pragma pack(pop)
void SetCurrentThreadName(const char* threadName)
{
    THREADNAME_INFO info;
    info.dwType = 0x1000;
    info.szName = threadName;
    info.dwThreadID = ::GetCurrentThreadId();
    info.dwFlags = 0;
#pragma warning(push)
#pragma warning(disable: 6320 6322)
    __try
    {
        RaiseException(MS_VC_EXCEPTION, 0, sizeof(info) / sizeof(ULONG_PTR), (ULONG_PTR*)&info);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
    }
#pragma warning(pop)
}
#elif __MACH__
void SetCurrentThreadName(const char* threadName)
{
    pthread_setname_np(threadName);
}
#else
void SetCurrentThreadName(const char* threadName)
{
    pthread_setname_np(pthread_self(), threadName);
}
#endif

void ThreadSleepForMsec(int msec)
{
#ifdef _WIN32
    ::Sleep(msec);
#else
    ::usleep(msec * 1000);
#endif
}


//------------------------------------------------------------------------------
// String Conversion

static const char* HEX_ASCII = "0123456789abcdef";

std::string HexString(uint64_t value)
{
    char hex[16 + 1];
    hex[16] = '\0';

    char* hexWrite = &hex[14];
    for (unsigned i = 0; i < 8; ++i)
    {
        hexWrite[1] = HEX_ASCII[value & 15];
        hexWrite[0] = HEX_ASCII[(value >> 4) & 15];

        value >>= 8;
        if (value == 0)
            return hexWrite;
        hexWrite -= 2;
    }

    return hex;
}

std::string HexDump(const uint8_t* data, int bytes)
{
    std::ostringstream oss;

    char hex[8 * 3];

    while (bytes > 8)
    {
        const uint64_t word = ReadU64_LE(data);

        for (unsigned i = 0; i < 8; ++i)
        {
            const uint8_t value = static_cast<uint8_t>(word >> (i * 8));
            hex[i * 3] = HEX_ASCII[(value >> 4) & 15];
            hex[i * 3 + 1] = HEX_ASCII[value & 15];
            hex[i * 3 + 2] = ' ';
        }

        oss.write(hex, 8 * 3);

        data += 8;
        bytes -= 8;
    }

    if (bytes > 0) {
        const uint64_t word = ReadBytes64_LE(data, bytes);

        for (int i = 0; i < bytes; ++i)
        {
            const uint8_t value = static_cast<uint8_t>(word >> (i * 8));
            hex[i * 3] = HEX_ASCII[(value >> 4) & 15];
            hex[i * 3 + 1] = HEX_ASCII[value & 15];
            hex[i * 3 + 2] = ' ';
        }

        oss.write(hex, bytes * 3);
    }

    return oss.str();
}


} // namespace kvm
