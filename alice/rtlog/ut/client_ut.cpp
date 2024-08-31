#include <alice/rtlog/client/client.h>
#include <alice/rtlog/common/eventlog/helpers.h>
#include <alice/rtlog/protos/rtlog.ev.pb.h>

#include <library/cpp/eventlog/common.h>
#include <library/cpp/eventlog/logparser.h>
#include <library/cpp/testing/unittest/registar.h>

#include <util/folder/iterator.h>
#include <util/folder/path.h>
#include <util/random/random.h>
#include <util/stream/file.h>
#include <util/system/event.h>
#include <util/system/fs.h>
#include <util/system/spinlock.h>

#include <thread>

using namespace NRTLog;

class TClientTest: public TTestBase {
private:
    UNIT_TEST_SUITE(TClientTest);
    UNIT_TEST(TestReopenLogsAsync);
    UNIT_TEST(TestReopenLogsSync);
    UNIT_TEST(TestAutoReopen);
    UNIT_TEST_SUITE_END();

public:
    void SetUp() override {
        NFs::RemoveRecursive(testDirectory);
    }

    void TearDown() override {
        NFs::RemoveRecursive(testDirectory);
    }

    void TestReopenLogsAsync() {
        DoTestReopenLogs({true}, true);
    }

    void TestReopenLogsSync() {
        DoTestReopenLogs({false}, true);
    }

    void TestAutoReopen() {
        DoTestReopenLogs({true, Nothing(), TDuration::MilliSeconds(1)}, false);
    }

private:
    struct TTestRequest {
        TVector<TString> Events;
        ui64 Index;
    };

private:
    void DoTestReopenLogs(const TClientOptions& clientOptions, bool callReopen) {
        constexpr size_t threadsCount = 10;
        constexpr size_t maxReopenFrequency = 100;
        constexpr size_t requestsCount = threadsCount * 1000;
        constexpr size_t maxEventsPerRequest = 10;
        constexpr size_t maxEventLength = 50;
        constexpr size_t maxOverlap = maxReopenFrequency * 3;

        ui64 startedRequestsCount = 0;
        TVector<TTestRequest> completedRequests;
        TVector<TString> filesWithEvents;

        NFs::MakeDirectory(testDirectory);
        const auto testFileName = testDirectory + "/rtlog-events";
        filesWithEvents.push_back(testFileName);
        {
            TClient client(testFileName, "test-client", clientOptions);
            TVector<std::thread> threads;
            TAdaptiveLock lock;
            TManualEvent goEvent;
            std::atomic<size_t> nextReopenIndex = maxReopenFrequency;

            for (size_t i = 0; i < threadsCount; ++i) {
                threads.push_back(std::thread([&]() {
                    Y_VERIFY(goEvent.Wait());
                    TMaybe<TTestRequest> testRequest;
                    while (true) {
                        ui64 requestIndex;
                        with_lock(lock) {
                            if (testRequest.Defined()) {
                                completedRequests.push_back(std::move(*testRequest));
                                testRequest.Clear();
                            }
                            requestIndex = startedRequestsCount;
                            if (requestIndex == requestsCount) {
                                break;
                            }
                            ++startedRequestsCount;
                            while (startedRequestsCount > nextReopenIndex + maxOverlap) {
                                auto u = Unguard(lock);
                                Sleep(TDuration::MilliSeconds(1));
                            }
                        }

                        auto logger = client.CreateRequestLogger(ToString(requestIndex));
                        const auto eventsCount = RandomNumber<ui32>(maxEventsPerRequest + 1);
                        testRequest.ConstructInPlace();
                        testRequest->Index = requestIndex;
                        for (size_t j = 0; j < eventsCount; ++j) {
                            NRTLogEvents::TLogEvent ev;
                            ev.SetSeverity(NRTLogEvents::RTLOG_SEVERITY_INFO);
                            const auto eventLength = RandomNumber<ui32>(maxEventLength + 1);
                            testRequest->Events.push_back(RandomEnglishLetters(eventLength));
                            ev.SetMessage(testRequest->Events.back());
                            logger->LogEvent(ev);
                        }
                        if (requestIndex == nextReopenIndex) {
                            const auto rotatedName = Sprintf("%s_%lu", testFileName.c_str(), filesWithEvents.size());
                            NFs::Rename(testFileName, rotatedName);
                            if (callReopen) {
                                client.Reopen();
                            }
                            while (!NFs::Exists(testFileName)) {
                                Sleep(TDuration::MilliSeconds(1));
                            }
                            filesWithEvents.push_back(rotatedName);
                            nextReopenIndex = startedRequestsCount + RandomNumber<ui32>(maxReopenFrequency);
                        }
                    }
                }));
            }

            goEvent.Signal();
            for (auto& t: threads) {
                t.join();
            }

            UNIT_ASSERT_VALUES_EQUAL(client.GetStats().ActiveLoggersCount, 0);
        }

        UNIT_ASSERT_VALUES_EQUAL(startedRequestsCount, requestsCount);
        UNIT_ASSERT_VALUES_EQUAL(completedRequests.size(), requestsCount);
        const auto writtenEvents = ReadEvents(filesWithEvents);
        CheckRequests(completedRequests, writtenEvents);
    }

    TVector<TTestRequest> ReadEvents(const TVector<TString>& files) {
        TVector<TTestRequest> result;
        for (const auto& f: files) {
            ReadEventsFromFile(f, result);
        }
        return result;
    }

    void ReadEventsFromFile(const TString& file, TVector<TTestRequest>& result) {
        TFileInput input(file);
        while (auto frame = FindNextFrame(&input, NEvClass::Factory())) {
            result.push_back({});
            auto& currentRequest = result.back();
            TFrameDecoder decoder(*frame, nullptr, false, true);
            TMaybe<ui64> index;
            while (true) {
                auto e = *decoder;
                if (!e || e->Class == TEndOfFrameEvent::EventClass) {
                    break;
                }
                if (e->Class == NRTLogEvents::LogEvent::ID) {
                    const auto& ev = Cast<NRTLogEvents::TLogEvent>(e.Get());
                    currentRequest.Events.push_back(ev.GetMessage());
                } else if (e->Class == NRTLogEvents::ActivationStarted::ID) {
                    const auto& ev = Cast<NRTLogEvents::TActivationStarted>(e.Get());
                    index = FromString<ui64>(ev.GetReqId());
                }
                if (!decoder.Next()) {
                    break;
                }
            }
            UNIT_ASSERT(index.Defined());
            currentRequest.Index = *index;
        }
    }

    TString RandomEnglishLetters(size_t size) {
        TString result;
        result.reserve(size);
        for (size_t i = 0; i < size; ++i) {
            result.append('a' + RandomNumber<char>('z' - 'a' + 1));
        }
        return result;
    }

    auto SortedByIndex(const TVector<TTestRequest>& requests) {
        TVector<TVector<TTestRequest>::const_iterator> iterators;
        iterators.reserve(requests.size());
        for (auto it = requests.begin(); it != requests.end(); ++it) {
            iterators.push_back(it);
        }
        SortBy(iterators, [](const auto& r) { return r->Index; });
        return iterators;
    }

    void CheckRequests(const TVector<TTestRequest>& expected, const TVector<TTestRequest>& actual) {
        UNIT_ASSERT_VALUES_EQUAL(expected.size(), actual.size());
        const auto sortedExpected = SortedByIndex(expected);
        const auto sortedActual = SortedByIndex(actual);
        for (size_t i = 0; i < sortedExpected.size(); ++i) {
            const auto& e = *sortedExpected[i];
            const auto& a = *sortedActual[i];
            UNIT_ASSERT_VALUES_EQUAL(e.Index, a.Index);
            UNIT_ASSERT_VALUES_EQUAL(e.Events.size(), a.Events.size());
            for (size_t j = 0; j < e.Events.size(); ++j) {
                UNIT_ASSERT_VALUES_EQUAL(e.Events[j], a.Events[j]);
            }
        }
    }

private:
    static const TString testDirectory;
};

const auto TClientTest::testDirectory = TString("./test-directory");

UNIT_TEST_SUITE_REGISTRATION(TClientTest);
