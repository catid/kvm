#include "kvm_logger.hpp"

#if !defined(ANDROID)
    #include <cstdio> // fwrite, stdout
#endif


namespace kvm {
namespace logger {


//------------------------------------------------------------------------------
// Level

static const char* kLevelStrings[(int)Level::Count] = {
    "tiny", "small", "reg", "large", "huge", "nope"
};

const char* LevelToString(Level level)
{
    static_assert((int)Level::Count == 6, "Update this switch");
    CORE_DEBUG_ASSERT((int)level >= 0 && level < Level::Count);
    return kLevelStrings[(int)level];
}

static const char kLevelChars[(int)Level::Count] = {
    't', 's', 'r', 'L', 'H', '-'
};

char LevelToChar(Level level)
{
    static_assert((int)Level::Count == 6, "Update this switch");
    CORE_DEBUG_ASSERT((int)level >= 0 && level < Level::Count);
    return kLevelChars[(int)level];
}


//------------------------------------------------------------------------------
// Log Callback

static LogCallback m_LogCallback = nullptr;

void SetLogCallback(LogCallback callback)
{
    // TBD: Not thread-safe
    m_LogCallback = callback;
}


//------------------------------------------------------------------------------
// OutputWorker

OutputWorker& OutputWorker::GetInstance()
{
    // Fix crash on shutdown due to MSVCRT bug prior to version 2015
    static std::once_flag m_singletonFlag;
    static std::unique_ptr<OutputWorker> instance;

    std::call_once(m_singletonFlag, []()
    {
        instance.reset(new OutputWorker);
    });

    return *instance.get();
}

#if !defined(LOGGER_DISABLE_ATEXIT)
static void AtExitWrapper()
{
    OutputWorker::GetInstance().Stop();
}
#endif // LOGGER_DISABLE_ATEXIT

OutputWorker::OutputWorker()
    : StartStopLock()
    , QueueLock()
    , QueueCondition()
    , QueuePublic()
    , QueuePrivate()
    , FlushCondition()
{
    // Start worker automatically on first reference
    Start();

#if !defined(LOGGER_DISABLE_ATEXIT)
    // Register an atexit() callback so we do not need manual shutdown in app code
    // Application code can still manually shutdown by calling OutputWorker::Stop()
    std::atexit(AtExitWrapper);
#endif // LOGGER_DISABLE_ATEXIT
}

void OutputWorker::Start()
{
    std::lock_guard<std::mutex> locker(StartStopLock);

    // If thread is already spinning:
    if (Thread) {
        return;
    }

#if defined(_WIN32)
    CachedIsDebuggerPresent = (::IsDebuggerPresent() != FALSE);
#endif // _WIN32

    QueuePublic.clear();
    QueuePrivate.clear();

#if !defined(LOGGER_NEVER_DROP)
    Overrun = 0;
#endif // LOGGER_NEVER_DROP
    FlushRequested = false;
    Terminated = false;
    Thread = std::make_shared<std::thread>(&OutputWorker::Loop, this);

#if defined(_WIN32)
    ThreadNativeHandle = Thread->native_handle();
#endif // _WIN32
}

void OutputWorker::Stop()
{
    std::lock_guard<std::mutex> locker(StartStopLock);

    if (Thread)
    {
#if defined(_WIN32)
        // If thread was force-terminated by something:
        if (WAIT_OBJECT_0 == ::WaitForSingleObject(ThreadNativeHandle, 0))
        {
            CORE_DEBUG_BREAK(); // Application force-terminated the thread
            return; // Abort shutdown...
        }
#endif // _WIN32

        Flush();

        Terminated = true;

        // Make sure that queue notification happens after termination flag is set
        {
            std::unique_lock<std::mutex> qlocker(QueueLock);
            QueueCondition.notify_all();
        }

        try
        {
            if (Thread->joinable()) {
                Thread->join();
            }
        }
        catch (std::system_error& /*err*/)
        {
        }
    }
    Thread = nullptr;

    // Make sure that concurrent Flush() calls do not block
    {
        std::unique_lock<std::mutex> qlocker(QueueLock);
        FlushCondition.notify_all();
    }
}

void OutputWorker::Write(LogStringBuffer& buffer)
{
    std::string str = buffer.LogStream.str();

#if defined(_WIN32)
    // If a debugger is present:
    if (CachedIsDebuggerPresent && !m_LogCallback)
    {
        // Log all messages immediately to the Visual Studio Output Window
        // to allow logging while single-stepping in a debugger.
        ::OutputDebugStringA((str + "\n").c_str());
    }
#endif // _WIN32

#if defined(LOGGER_NEVER_DROP)
    for (;; Flush())
    {
        std::lock_guard<std::mutex> locker(QueueLock);

        if (QueuePublic.size() >= kWorkQueueLimit)
        {
            continue;
        }
        else
        {
            QueuePublic.emplace_back(buffer.LogLevel, buffer.ChannelName, str);
            break;
        }
    }
#else // LOGGER_NEVER_DROP
    {
        std::lock_guard<std::mutex> locker(QueueLock);

        if (QueuePublic.size() >= kWorkQueueLimit) {
            Overrun++;
        }
        else {
            QueuePublic.emplace_back(buffer.LogLevel, buffer.ChannelName, str);
        }
    }
#endif // LOGGER_NEVER_DROP

    QueueCondition.notify_all();
}

void OutputWorker::Loop()
{
    SetCurrentThreadName("Logger");

    while (!Terminated)
    {
        int overrun = 0;
        bool flushRequested = false;
        {
            // unique_lock used since QueueCondition.wait requires it
            std::unique_lock<std::mutex> locker(QueueLock);

            if (QueuePublic.empty() && !FlushRequested && !Terminated) {
                QueueCondition.wait(locker);
            }

            std::swap(QueuePublic, QueuePrivate);

#if !defined(LOGGER_NEVER_DROP)
            overrun = Overrun;
            Overrun = 0;
#endif // LOGGER_NEVER_DROP

            flushRequested = FlushRequested;
            FlushRequested = false;
        }

        for (auto& log : QueuePrivate) {
            Log(log);
        }

        // Handle log message overrun
        if (overrun > 0)
        {
            std::ostringstream oss;
            oss << "Queue overrun. Lost " << overrun << " log messages";
            std::string str = oss.str();
            QueuedMessage qm(Level::Error, "Logger", str);
            Log(qm);
        }

        QueuePrivate.clear();

        if (flushRequested) {
            FlushCondition.notify_all();
        }

        EndLogFlush();
    }

#if 0
    // Log out that logger is terminating
    std::string terminatingMessage = "Terminating";
    QueuedMessage qm(Level::Info, "Logger", terminatingMessage);
    Log(qm);
#endif
}

void OutputWorker::Log(QueuedMessage& message)
{
    std::ostringstream ss;
    ss << '{' << LevelToChar(message.LogLevel);
    if (message.LogLevel == Level::Error) {
        ss << "-ERR"; // Make errors searchable
    } else if (message.LogLevel == Level::Warn) {
        ss << "-WARN"; // Make warnings searchable
    }
    ss << '-' << message.ChannelName << "} " << message.Message;

    if (m_LogCallback) {
        ss << std::endl;
        std::string fmtstr = ss.str();
        m_LogCallback(message, fmtstr);
        return;
    }

#if defined(ANDROID)
    std::string fmtstr = ss.str();
    __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "%s", fmtstr.c_str());
#else // ANDROID
    ss << std::endl;
    std::string fmtstr = ss.str();
    fwrite(fmtstr.c_str(), 1, fmtstr.size(), stdout);
#endif // ANDROID
}

void OutputWorker::EndLogFlush()
{
    // Flush stdout to e.g. get journalctl on linux to update
    fflush(stdout);
}

void OutputWorker::Flush()
{
    // unique_lock used since FlushCondition.wait requires it
    std::unique_lock<std::mutex> locker(QueueLock);

    if (!Terminated)
    {
        FlushRequested = true;
        QueueCondition.notify_all();

        FlushCondition.wait(locker);
    }
}


//------------------------------------------------------------------------------
// Channel

Channel::Channel(const char* name, Level minLevel)
    : ChannelName(name)
    , ChannelMinLevel(minLevel)
    , PrefixLock()
    , Prefix()
{
}

std::string Channel::GetPrefix() const
{
    std::lock_guard<std::mutex> locker(PrefixLock);
    return Prefix;
}

void Channel::SetPrefix(const std::string& prefix)
{
    std::lock_guard<std::mutex> locker(PrefixLock);
    Prefix = prefix;
}


} // namespace logger
} // namespace kvm


// Fix hang on shutdown due to MSVCRT bug prior to version 2015
#if defined(_MSC_VER) && (_MSC_VER < 1900)

// https://stackoverflow.com/questions/10915233/stdthreadjoin-hangs-if-called-after-main-exits-when-using-vs2012-rc
#pragma warning(disable:4073) // initializers put in library initialization area
#pragma init_seg(lib)

struct VS2013_threading_fix
{
    VS2013_threading_fix()
    {
        _Cnd_do_broadcast_at_thread_exit();
    }
} threading_fix;

#endif // _MSC_VER
