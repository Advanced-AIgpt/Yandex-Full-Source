#include "service.h"
#include "converters.h"
#include "utils.h"

#include <alice/cuttlefish/library/cuttlefish/common/common_items.h>
#include <alice/cuttlefish/library/cuttlefish/common/item_types.h>

#include <alice/cuttlefish/library/logging/dlog.h>

#include <alice/cuttlefish/library/protos/audio.pb.h>
#include <alice/cuttlefish/library/protos/events.pb.h>
#include <alice/cuttlefish/library/protos/session.pb.h>

#include <util/string/builder.h>

namespace NAlice::NCuttlefish::NAppHostServices {

using namespace NAlice::NCuttlefish::NAppHostServices::NConverter;

namespace {

template <typename MessageT>
inline MessageT Parse(const NAppHost::NService::TProtobufItem& it) {
    MessageT msg;
    it.Fill(&msg);
    return msg;
}

}

bool RawToProtobufImpl(NAppHost::IServiceContext& ctx, TLogContext logContext, NAliceProtocol::TEvent& event)
{
    using namespace NAliceProtocol;

    if (!ctx.HasProtobufItem(ITEM_TYPE_WS_MESSAGE)) {
        logContext.LogEvent(NEvClass::WarningMessage("Item ws_message was not found"));
        return false;
    }

    TWsEvent rawMessage = ctx.GetOnlyProtobufItem<TWsEvent>(ITEM_TYPE_WS_MESSAGE);

    Y_ENSURE(rawMessage.HasHeader(), "TWsEvent must have header");
    const TEventHeader& header = rawMessage.GetHeader();

    const TStringBuf text = rawMessage.GetText();
    Y_ENSURE(!text.Empty(), "Text must not be empty");
    logContext.LogEvent(
        NEvClass::InfoMessage(
            TStringBuilder()
                << "Namespace=" << TEventHeader::EMessageNamespace_Name(header.GetNamespace()) << "; "
                << "Name=" << TEventHeader::EMessageName_Name(header.GetName()) << "; "
                << "MessageId=" << header.GetMessageId() << "; "
                << "Text:\n" << text
        )
    );

    try {
        switch (header.GetName()) {
            case TEventHeader::SYNCHRONIZE_STATE:
                logContext.LogEvent(NEvClass::InfoMessage("Convert SynchronizeState..."));
                *event.MutableSyncState() = JsonToProtobuf(GetSynchronizeStateEventConverter(), ReadJsonValue(text));
                *event.MutableHeader() = header;
                return true;
            default:
                logContext.LogEvent(
                    NEvClass::WarningMessage(
                        TStringBuilder()
                            << "Unknown message: "
                            << TEventHeader::EMessageName_Name(header.GetName())
                    )
                );
                return false;
        }

    } catch (const NJson::TJsonException& exc) {
        logContext.LogEvent(NEvClass::WarningMessage(TStringBuilder() << "TJsonException: " << exc.what()));
        ctx.AddProtobufItem(
            CreateEventExceptionEx("SYNCHRONIZE_STATE_PRE", "", "Incorrect JSON", header.GetMessageId()),
            ITEM_TYPE_DIRECTIVE
        );
    } catch (const TConvertException& exc) {
        logContext.LogEvent(NEvClass::WarningMessage(TStringBuilder() << "TConvertException: " << exc.what()));
        ctx.AddProtobufItem(
            CreateEventExceptionEx("SYNCHRONIZE_STATE_PRE", "", exc.what(), header.GetMessageId()),
            ITEM_TYPE_DIRECTIVE
        );
    }
    return false;
}

void RawToProtobuf(NAppHost::IServiceContext& ctx, TLogContext logContext)
{
    NAliceProtocol::TEvent event;
    if (RawToProtobufImpl(ctx, logContext, event)) {
        ctx.AddProtobufItem(std::move(event), ITEM_TYPE_SYNCRHONIZE_STATE_EVENT);
    }
}


void ProtobufToRawImpl(NAppHost::IServiceContext& ctx, const NAliceProtocol::TDirective& directive)
{
    NAliceProtocol::TWsEvent wsMessage;
    wsMessage.SetText(ProtobufToJson(GetDirectiveConverter(), directive));
    *wsMessage.MutableHeader() = directive.GetHeader();
    ctx.AddProtobufItem(wsMessage, ITEM_TYPE_WS_MESSAGE);
}

void ProtobufToRaw(NAppHost::IServiceContext& ctx, TLogContext logContext)
{
    const auto items = ctx.GetProtobufItemRefs(NAppHost::EContextItemSelection::Input);
    for (auto it = items.begin(); it != items.end(); ++it) {
        const TStringBuf type = it.GetType();
        logContext.LogEvent(NEvClass::InfoMessage(TStringBuilder() << "Convert '" << it.GetTag() << "@" << type << "' to TWsEvent..."));

        if (type == ITEM_TYPE_DIRECTIVE) {
            ProtobufToRawImpl(ctx, Parse<NAliceProtocol::TDirective>(*it));
        }
    }
}

}  // namespace NAlice::NCuttlefish::NAppHostServices
