#pragma once

#include <mapreduce/yt/common/config.h>

namespace NAlice {

//Adds fields to the row, and returns false if the message doesn't contains needed information
bool TryParseLogMessage(TStringBuf messageBuf, NYT::TNode& row);

//Parses message with "ACCESS" type
/*String example:
"ACCESSLOG: sas1-0223-sas-uniproxy-20988.gencfg-c.yandex.net 83.149.21.76 \"POST /asr_full 1/1\" 200 \"ea39f01e-ae78-4f57-b91f-d9024983616e\" \"629f3d6cd6dd30fa7ea62a76873841b1\" \"5ae68c9c-bd1e-4e3d-b63c-40010d8bfbd7\" \"cc96633d-59d4-4724-94bd-f5db2f02ad13\" 0 dialogeneral+dialog-general-gpu 0 \"2.5103163719177246\" 5831\n"*/
bool TryParseAccessLogMessage(TStringBuf messageBuf, NYT::TNode& row);

//Parses message with "SESSION" type, only message with "VinsRequest" are interesting
bool TryParseSessionLogMessage(TStringBuf messageBuf, NYT::TNode& row);

} // namespace NAlice
