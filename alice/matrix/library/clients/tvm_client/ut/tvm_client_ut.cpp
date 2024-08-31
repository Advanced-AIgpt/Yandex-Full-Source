#include <alice/matrix/library/clients/tvm_client/tvm_client.h>

#include <alice/matrix/library/logging/log_context.h>

#include <library/cpp/testing/common/env.h>
#include <library/cpp/testing/common/network.h>
#include <library/cpp/testing/gtest/gtest.h>

#include <util/stream/file.h>
#include <util/string/cast.h>


namespace {

class TMatrixTvmClientTestBase : public ::testing::Test {
protected:
    TMatrixTvmClientTestBase()
        : ::testing::Test()
        , TvmApiPort_(GetPortFromFile(TVM_API_PORT_FILE_NAME))
        , TvmToolPort_(GetPortFromFile(TVM_TOOL_PORT_FILE_NAME))
        , TvmToolAuthToken_(GetFileData(TVM_TOOL_AUTHTOKEN_FILE_NAME))
        , FreePort_(NTesting::GetFreePort())
        , OutputDir_(CreateAndGetOutputDir())
    {}

    void SetUp() override {
        NMatrix::TLogger::TConfig loggerConfig;
        loggerConfig.Filename = (OutputDir_ / TFsPath(TVM_CLIENT_LOG_FILE)).GetPath();
        NMatrix::GetLogger().Init(loggerConfig);
    }

    virtual NMatrix::TTvmClientSettings GetConfig() const = 0;
    virtual NMatrix::TTvmClientSettings GetWrongPortConfig() const = 0;

    void TestNotInitialized() {
        NMatrix::TTvmClient tvmClient(GetWrongPortConfig());

        EXPECT_FALSE(tvmClient.EnsureInitializedAndReady());

        auto serviceTicket = tvmClient.GetServiceTicketFor("one");
        EXPECT_FALSE(tvmClient.EnsureInitializedAndReady());
        ASSERT_TRUE(serviceTicket.IsError());
        EXPECT_EQ(serviceTicket.Error(), "Client is not initialized");
    }

    void TestLazyInitializationIsThreadSafe() {
        NMatrix::TTvmClient tvmClient(GetWrongPortConfig());

        TThreadPool threadPool;
        threadPool.Start(5);

        TVector<NThreading::TFuture<void>> futures;
        for (size_t i = 0; i < 100; ++i) {
            futures.emplace_back(
                NThreading::Async(
                    [i, &tvmClient]() {
                        if (i & 1) {
                            EXPECT_FALSE(tvmClient.EnsureInitializedAndReady());
                        } else {
                            EXPECT_TRUE(tvmClient.GetServiceTicketFor("one").IsError());
                        }
                    },
                    threadPool
                )
            );
        }

        NThreading::WaitAll(futures).GetValueSync();
        threadPool.Stop();
    }

    void TestGetServiceTicketFor() {
        NMatrix::TTvmClient tvmClient(GetConfig());

        EXPECT_TRUE(tvmClient.EnsureInitializedAndReady());

        {
            auto serviceTicket = tvmClient.GetServiceTicketFor("one");
            EXPECT_TRUE(tvmClient.EnsureInitializedAndReady());
            ASSERT_TRUE(serviceTicket.IsSuccess());
            EXPECT_THAT(serviceTicket.Success().Ticket, ::testing::StartsWith("3:serv:"));
        }

        {
            auto serviceTicket = tvmClient.GetServiceTicketFor("trash");
            EXPECT_TRUE(tvmClient.EnsureInitializedAndReady());
            ASSERT_TRUE(serviceTicket.IsError());
            EXPECT_THAT(serviceTicket.Error(), ::testing::HasSubstr("Destination 'trash' was not specified in settings"));
        }
    }

private:
    TFsPath CreateAndGetOutputDir() {
        const auto* testInfo = ::testing::UnitTest::GetInstance()->current_test_info();
        const TString outputSubdir = TString::Join(
            testInfo->test_suite_name(),
            '_',
            testInfo->name()
        );

        const TFsPath outputDir = GetOutputPath() / TFsPath(outputSubdir);
        NFs::MakeDirectoryRecursive(outputDir.GetPath());

        return outputDir;
    }

private:
    static TString GetFileData(const TString& fileName) {
        return TUnbufferedFileInput(fileName).ReadAll();
    }

    static ui32 GetPortFromFile(const TString& fileName) {
        return FromString<ui32>(GetFileData(fileName));
    }

private:
    static inline const TString TVM_CLIENT_LOG_FILE = "tvm_client_eventlog";
    static inline const TString TVM_API_PORT_FILE_NAME = "tvmapi.port";
    static inline const TString TVM_TOOL_PORT_FILE_NAME = "tvmtool.port";
    static inline const TString TVM_TOOL_AUTHTOKEN_FILE_NAME = "tvmtool.authtoken";

protected:
    const ui32 TvmApiPort_;
    const ui32 TvmToolPort_;
    const TString TvmToolAuthToken_;
    const NTesting::TPortHolder FreePort_;
    const TFsPath OutputDir_;
};

class TMatrixTvmClientTvmApiTest : public TMatrixTvmClientTestBase {
protected:
    NMatrix::TTvmClientSettings GetConfig() const override {
        NMatrix::TTvmClientSettings config;
        {
            // Do not change tvm ids and secret
            // These are the magic consts from tvm api recipe
            // https://a.yandex-team.ru/svn/trunk/arcadia/library/recipes/tvmapi/clients/clients.json?rev=r6857921#L2-10

            NMatrix::TTvmClientSettings::TTvmApiSettings& tvmApiConfig = *config.MutableTvmApi();
            tvmApiConfig.SetHost("localhost");
            tvmApiConfig.SetPort(TvmApiPort_);
            tvmApiConfig.SetSelfTvmId(1000501);

            tvmApiConfig.SetDiskCacheDir(OutputDir_.GetPath());

            {
                auto& tvmIdAlias = *tvmApiConfig.AddFetchServiceTicketsFor();
                tvmIdAlias.SetAlias("one");
                tvmIdAlias.SetTvmId(1000502);
            }
            {
                auto& tvmIdAlias = *tvmApiConfig.AddFetchServiceTicketsFor();
                tvmIdAlias.SetAlias("two");
                tvmIdAlias.SetTvmId(1000503);
            }

            tvmApiConfig.SetPlainTextSecret("bAicxJVa5uVY7MjDlapthw");
        }

        return config;
    }

    NMatrix::TTvmClientSettings GetWrongPortConfig() const override {
        NMatrix::TTvmClientSettings config = GetConfig();
        config.MutableTvmApi()->SetPort(FreePort_);

        return config;
    }
};

class TMatrixTvmClientTvmToolTest : public TMatrixTvmClientTestBase {
protected:
    NMatrix::TTvmClientSettings GetConfig() const override {
        NMatrix::TTvmClientSettings config;
        {
            NMatrix::TTvmClientSettings::TTvmToolSettings& tvmToolConfig = *config.MutableTvmTool();
            tvmToolConfig.SetPort(TvmToolPort_);
            tvmToolConfig.SetSelfAlias("self_v1");
            tvmToolConfig.SetAuthToken(TvmToolAuthToken_);
        }

        return config;
    }

    NMatrix::TTvmClientSettings GetWrongPortConfig() const override {
        NMatrix::TTvmClientSettings config = GetConfig();
        config.MutableTvmTool()->SetPort(FreePort_);

        return config;
    }
};

} // namespace


TEST(TMatrixTvmClientTest, BadConfig) {
    EXPECT_THROW_MESSAGE_HAS_SUBSTR(
        []() {
            NMatrix::TTvmClientSettings badConfig;
            NMatrix::TTvmClient tvmClient(badConfig);
        }(),
        yexception,
        "Tvm client mode not set"
    );
}

TEST_F(TMatrixTvmClientTvmApiTest, TestNotInitialized) {
    TestNotInitialized();
}

TEST_F(TMatrixTvmClientTvmApiTest, TestLazyInitializationIsThreadSafe) {
    TestLazyInitializationIsThreadSafe();
}

TEST_F(TMatrixTvmClientTvmApiTest, TestGetServiceTicketFor) {
    TestGetServiceTicketFor();
}

TEST_F(TMatrixTvmClientTvmToolTest, TestNotInitialized) {
    TestNotInitialized();
}

TEST_F(TMatrixTvmClientTvmToolTest, TestLazyInitializationIsThreadSafe) {
    TestLazyInitializationIsThreadSafe();
}

TEST_F(TMatrixTvmClientTvmToolTest, TestGetServiceTicketFor) {
    TestGetServiceTicketFor();
}
