#pragma once

#include <alice/uniproxy/mapper/library/flags/container.h>
#include <alice/uniproxy/mapper/library/logging/logging.h>
#include <alice/uniproxy/mapper/library/sensors/constants.h>

#include <util/datetime/base.h>
#include <util/generic/array_ref.h>
#include <util/generic/map.h>
#include <util/generic/string.h>

#include <memory>

namespace Poco {
    template <class T>
    class Buffer;
}

namespace Poco::Net {
    class HTTPClientSession;
    class WebSocket;
}

namespace NAlice::NUniproxy {
    /// \brief The low-level client parameters
    struct TBaseClientParams {
        TDuration ConnectTimeout = TDuration::Seconds(1);  //!< The connect timeout
        TDuration SendTimeout = TDuration::Seconds(3);     //!< The send timeout
        TDuration ReceiveTimeout = TDuration::Seconds(10); //!< The receive timeout
        bool DisableServerCertificateValidation = false;   //!< The server certificate validation flag
        TMap<TString, TString> SessionHeaders;             //!< Headers which are passed on websocket connection
        TLog* Logger = nullptr;                            //!< The log interface, optional
        TFlagsContainer* FlagsContainer = nullptr;         //!< Feature flag interface
        TSensorContainer* Sensors = nullptr;               //!< Solomon metrics
    };

    /// \brief The low-level Uniproxy client
    class TBaseClient {
    private:
        TLog* Logger;
        const TFlagsContainer* FlagsContainer;
        std::unique_ptr<Poco::Net::HTTPClientSession> HttpSession;
        std::unique_ptr<Poco::Net::WebSocket> WebSocket;

    public:
        /// @brief The low-level receive result type
        enum class EResultType {
            None,  //<! No data received (connection closed)
            Text,  //<! The text response
            Binary //<! The binary response
        };

        /// @brief The low-level receive result info
        struct TReceiveResult {
            size_t Size = 0;                      //!< The response size
            EResultType Type = EResultType::None; //!< The response type
        };
        const TBaseClientParams Params;

    public:
        /** @brief Constructs a new low-level client
         *
         * @param uniproxyUri The uniproxy URL
         * @param params Additional parameters
         */
        explicit TBaseClient(TStringBuf uniproxyUri, const TBaseClientParams& params = {});
        ~TBaseClient();

        /** @brief Sends a text frame
         *
         * @param text The text frame data
         */
        void SendText(TStringBuf text);
        /** @brief Sends a binary frame
         *
         * @param data The binary frame data
         */
        void SendBinary(TArrayRef<char> data);

        /** @brief Receives a frame
         *
         * @param buffer A buffer to store a received data
         * @return The low-level receive result info
         */
        TReceiveResult Receive(Poco::Buffer<char>& buffer);

        /// @brief Returns the remote IP address
        TString GetRemoteAddress() const;
        /// @brief Returns the remote port
        ui16 GetRemotePort() const;
    };

}
