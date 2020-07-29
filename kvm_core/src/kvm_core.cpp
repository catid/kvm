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


//------------------------------------------------------------------------------
// Aligned Allocation

static const unsigned kAlignBytes = 32;

uint8_t* AlignedAllocate(size_t size)
{
    uint8_t* data = (uint8_t*)malloc(kAlignBytes + size);
    if (!data) {
        return nullptr;
    }
    unsigned offset = (unsigned)((uintptr_t)data % kAlignBytes);
    data += kAlignBytes - offset;
    data[-1] = (uint8_t)offset;
    return data;
}

void AlignedFree(void* p)
{
    if (!p) {
        return;
    }
    uint8_t* data = (uint8_t*)p;
    unsigned offset = data[-1];
    if (offset > kAlignBytes) {
        return;
    }
    data -= kAlignBytes - offset;
    free(data);
}


//------------------------------------------------------------------------------
// Conversion to Base64

static const char TO_BASE64[64] = {
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
    'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
    'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
    'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
    'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
    'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
    'w', 'x', 'y', 'z', '0', '1', '2', '3',
    '4', '5', '6', '7', '8', '9', '+', '/'
};


int GetBase64LengthFromByteCount(int bytes)
{
    if (bytes <= 0) {
        return 0;
    }

    return ((bytes + 2) / 3) * 4;
}

int WriteBase64(
    const void* buffer,
    int bytes,
    char* encoded_buffer,
    int encoded_bytes)
{
    int written_bytes = ((bytes + 2) / 3) * 4;

    if (bytes <= 0 || encoded_bytes < written_bytes) {
        return 0;
    }

    const uint8_t *data = reinterpret_cast<const uint8_t*>( buffer );

    int ii, jj, end;
    for (ii = 0, jj = 0, end = bytes - 2; ii < end; ii += 3, jj += 4)
    {
        encoded_buffer[jj] = TO_BASE64[data[ii] >> 2];
        encoded_buffer[jj+1] = TO_BASE64[((data[ii] << 4) | (data[ii+1] >> 4)) & 0x3f];
        encoded_buffer[jj+2] = TO_BASE64[((data[ii+1] << 2) | (data[ii+2] >> 6)) & 0x3f];
        encoded_buffer[jj+3] = TO_BASE64[data[ii+2] & 0x3f];
    }

    switch (ii - end)
    {
        default:
        case 2: // Nothing to write
            break;

        case 1: // Need to write final 1 byte
            encoded_buffer[jj] = TO_BASE64[data[bytes-1] >> 2];
            encoded_buffer[jj+1] = TO_BASE64[(data[bytes-1] << 4) & 0x3f];
            encoded_buffer[jj+2] = '=';
            encoded_buffer[jj+3] = '=';
            break;

        case 0: // Need to write final 2 bytes
            encoded_buffer[jj] = TO_BASE64[data[bytes-2] >> 2];
            encoded_buffer[jj+1] = TO_BASE64[((data[bytes-2] << 4) | (data[bytes-1] >> 4)) & 0x3f];
            encoded_buffer[jj+2] = TO_BASE64[(data[bytes-1] << 2) & 0x3f];
            encoded_buffer[jj+3] = '=';
            break;
    }
    return written_bytes;
}

// This version writes a C string null-terminator
int WriteBase64Str(
    const void* buffer,
    int bytes,
    char* encoded_buffer,
    int encoded_bytes)
{
    const int written = WriteBase64(
        buffer,
        bytes,
        encoded_buffer,
        encoded_bytes - 1);

    encoded_buffer[written] = '\0';

    return written;
}

std::string BinToBase64(const void* data, int bytes)
{
    const int bc = GetBase64LengthFromByteCount(bytes);
    std::vector<char> text(bc + 1);
    WriteBase64Str(data, bytes, text.data(), bc);
    return text.data();
}


//------------------------------------------------------------------------------
// Conversion from Base64

#define DC 0

static const uint8_t FROM_BASE64[256] = {
    DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, // 0-15
    DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, // 16-31
    DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, 62, DC, DC, DC, 63, // 32-47
    52, 53, 54, 55, 56, 57, 58, 59, 60, 61, DC, DC, DC, DC, DC, DC, // 48-63
    DC,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, // 64-79
    15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, DC, DC, DC, DC, DC, // 80-95
    DC, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, // 96-111
    41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, DC, DC, DC, DC, DC, // 112-127
    DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, // 128-
    DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, // Extended
    DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, // ASCII
    DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC,
    DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC,
    DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC,
    DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC,
    DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC
};

#undef DC

int GetByteCountFromBase64(const char* encoded_buffer, int bytes)
{
    if (bytes <= 0) return 0;

    // Skip characters from end until one is a valid BASE64 character
    while (bytes >= 1) {
        unsigned char ch = encoded_buffer[bytes - 1];

        if (FROM_BASE64[ch] != 0 || ch == 'A') {
            break;
        }

        --bytes;
    }

    // Round down because we pad out the high bits with zeros
    return (bytes * 6) / 8;
}

int ReadBase64(
    const char* encoded_buffer,
    int encoded_bytes,
    void* decoded_buffer)
{
    // Skip characters from end until one is a valid BASE64 character
    while (encoded_bytes >= 1) {
        unsigned char ch = encoded_buffer[encoded_bytes - 1];

        if (FROM_BASE64[ch] != 0 || ch == 'A') {
            break;
        }

        --encoded_bytes;
    }

    if (encoded_bytes <= 0) {
        return 0;
    }

    const uint8_t* from = reinterpret_cast<const uint8_t*>( encoded_buffer );
    uint8_t* to = reinterpret_cast<uint8_t*>( decoded_buffer );

    int ii, jj, ii_end = encoded_bytes - 3;
    for (ii = 0, jj = 0; ii < ii_end; ii += 4, jj += 3)
    {
        const uint8_t a = FROM_BASE64[from[ii]];
        const uint8_t b = FROM_BASE64[from[ii+1]];
        const uint8_t c = FROM_BASE64[from[ii+2]];
        const uint8_t d = FROM_BASE64[from[ii+3]];

        to[jj] = (a << 2) | (b >> 4);
        to[jj+1] = (b << 4) | (c >> 2);
        to[jj+2] = (c << 6) | d;
    }

    switch (encoded_bytes & 3)
    {
    case 3: // 3 characters left
        {
            const uint8_t a = FROM_BASE64[from[ii]];
            const uint8_t b = FROM_BASE64[from[ii+1]];
            const uint8_t c = FROM_BASE64[from[ii+2]];

            to[jj] = (a << 2) | (b >> 4);
            to[jj+1] = (b << 4) | (c >> 2);
            return jj + 2;
        }
    case 2: // 2 characters left
        {
            const uint8_t a = FROM_BASE64[from[ii]];
            const uint8_t b = FROM_BASE64[from[ii+1]];

            to[jj] = (a << 2) | (b >> 4);
            return jj + 1;
        }
    }

    return jj;
}

void Base64ToBin(const std::string& base64, std::vector<uint8_t>& data)
{
    const int chars = (int)base64.size();
    const int bc = GetByteCountFromBase64(base64.c_str(), chars);
    data.resize(bc);
    ReadBase64(base64.c_str(), chars, data.data());
}


} // namespace kvm
