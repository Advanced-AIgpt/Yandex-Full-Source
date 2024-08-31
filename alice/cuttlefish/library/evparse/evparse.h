#pragma once

#include <util/generic/string.h>
#include <util/generic/strbuf.h>

#include <alice/cuttlefish/library/protos/wsevent.pb.h>
#include <alice/cuttlefish/library/evparse/evparse.h>


bool ParseEvent(TStringBuf data, NAliceProtocol::TEventHeader& header);

bool ParseEventFast(TStringBuf data, NAliceProtocol::TEventHeader& header);
