#pragma once

#include "messages.h"
#include <alice/nlu/granet/lib/utils/text_view.h>
#include <alice/nlu/granet/lib/utils/utils.h>

namespace NGranet::NCompiler {

// ~~~~ TCompilerError ~~~~

class TCompilerError : public yexception {
public:
    TString Message;
    TFsPath FileName;

    // Position of error in source text. Position is measured in characters (not utf-8 bytes).
    bool IsPositionDefined = false;
    size_t LineIndex = 0;
    size_t ColumnIndex = 0;
    size_t CharCount = 0;

    // For testing.
    EMessageId MessageId = MSG_UNDEFINED;

public:
    explicit TCompilerError(TStringBuf message, EMessageId messageId)
        : Message(TString{message})
        , MessageId(messageId)
    {
        PrintMessage();
    }

    TCompilerError(const TTextView& source, TStringBuf message, EMessageId messageId)
        : Message(TString{message})
        , IsPositionDefined(source.IsDefined())
        , MessageId(messageId)
    {
        if (source.IsDefined()) {
            FileName = source.GetSourceText()->Path;
            source.GetCoordinates(&LineIndex, &ColumnIndex, &CharCount);
        }
        PrintMessage();
        if (source.IsDefined()) {
            *this << source.PrintErrorPosition();
        }
    }

private:
    void PrintMessage() {
        *this << "Grammar compiler error!" << Endl;
        *this << Message << Endl;
    }
};

} // namespace NGranet::NCompiler
