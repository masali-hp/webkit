// Use a background thread to periodically scavenge memory to release back to the system
#if PLATFORM(IOS)
#define USE_BACKGROUND_THREAD_TO_SCAVENGE_MEMORY 0
#else
#define USE_BACKGROUND_THREAD_TO_SCAVENGE_MEMORY 1
#endif

#ifndef NDEBUG
namespace WTF {

#if OS(WINDOWS)

// TLS_OUT_OF_INDEXES is not defined on WinCE.
#ifndef TLS_OUT_OF_INDEXES
#define TLS_OUT_OF_INDEXES 0xffffffff
#endif

static DWORD isForibiddenTlsIndex = TLS_OUT_OF_INDEXES;
static const LPVOID kTlsAllowValue = reinterpret_cast<LPVOID>(0); // Must be zero.
static const LPVOID kTlsForbiddenValue = reinterpret_cast<LPVOID>(1);

#if !ASSERT_DISABLED
static bool isForbidden()
{
    // By default, fastMalloc is allowed so we don't allocate the
    // tls index unless we're asked to make it forbidden. If TlsSetValue
    // has not been called on a thread, the value returned by TlsGetValue is 0.
    return (isForibiddenTlsIndex != TLS_OUT_OF_INDEXES) && (TlsGetValue(isForibiddenTlsIndex) == kTlsForbiddenValue);
}
#endif

void fastMallocForbid()
{
    if (isForibiddenTlsIndex == TLS_OUT_OF_INDEXES)
        isForibiddenTlsIndex = TlsAlloc(); // a little racey, but close enough for debug only
    TlsSetValue(isForibiddenTlsIndex, kTlsForbiddenValue);
}

void fastMallocAllow()
{
    if (isForibiddenTlsIndex == TLS_OUT_OF_INDEXES)
        return;
    TlsSetValue(isForibiddenTlsIndex, kTlsAllowValue);
}

#else // !OS(WINDOWS)

static pthread_key_t isForbiddenKey;
static pthread_once_t isForbiddenKeyOnce = PTHREAD_ONCE_INIT;
static void initializeIsForbiddenKey()
{
  pthread_key_create(&isForbiddenKey, 0);
}

#if !ASSERT_DISABLED
static bool isForbidden()
{
    pthread_once(&isForbiddenKeyOnce, initializeIsForbiddenKey);
    return !!pthread_getspecific(isForbiddenKey);
}
#endif

void fastMallocForbid()
{
    pthread_once(&isForbiddenKeyOnce, initializeIsForbiddenKey);
    pthread_setspecific(isForbiddenKey, &isForbiddenKey);
}

void fastMallocAllow()
{
    pthread_once(&isForbiddenKeyOnce, initializeIsForbiddenKey);
    pthread_setspecific(isForbiddenKey, 0);
}
#endif // OS(WINDOWS)

} // namespace WTF
#endif // NDEBUG

namespace WTF {


namespace Internal {
#if !ENABLE(WTF_MALLOC_VALIDATION)
WTF_EXPORT_PRIVATE void fastMallocMatchFailed(void*);
#else
COMPILE_ASSERT(((sizeof(ValidationHeader) % sizeof(AllocAlignmentInteger)) == 0), ValidationHeader_must_produce_correct_alignment);
#endif

NO_RETURN_DUE_TO_CRASH void fastMallocMatchFailed(void*)
{
    CRASH();
}

} // namespace Internal

void* fastZeroedMalloc(size_t n)
{
    void* result = fastMalloc(n);
    memset(result, 0, n);
    return result;
}

char* fastStrDup(const char* src)
{
    size_t len = strlen(src) + 1;
    char* dup = static_cast<char*>(fastMalloc(len));
    memcpy(dup, src, len);
    return dup;
}

TryMallocReturnValue tryFastZeroedMalloc(size_t n)
{
    void* result;
    if (!tryFastMalloc(n).getValue(result))
        return 0;
    memset(result, 0, n);
    return result;
}

size_t fastMallocGoodSize(size_t bytes)
{
#if OS(DARWIN)
    return malloc_good_size(bytes);
#else
    return bytes;
#endif
}

} // namespace WTF