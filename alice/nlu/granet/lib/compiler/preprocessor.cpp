#include "preprocessor.h"
#include "compiler_check.h"
#include "syntax.h"
#include <util/generic/is_in.h>
#include <util/generic/stack.h>
#include <util/string/builder.h>
#include <util/string/escape.h>
#include <util/string/strip.h>

namespace NGranet::NCompiler {

static const TStringBuf LEFT_BRACKETS = "([{<";
static const TStringBuf RIGHT_BRACKETS = ")]}>";

// ~~~~ TSourcesLoader ~~~~

TString TSourcesLoader::ReadFileAsString(const TTextView& source, const TFsPath& path) {
    GRANET_COMPILER_CHECK(path.IsRelative(), source, MSG_ABSOLUTE_PATH, path);
    TFsPath fixed = path;
    fixed.Fix();

    TFsDataLoader defaultLoader;
    IDataLoader& loader = Loader != nullptr ? *Loader : defaultLoader;

    for (const TFsPath& dir : SourceDirs) {
        TFsPath full = dir / fixed;
        full.Fix();
        if (!loader.IsFile(full)) {
            continue;
        }
        const TString text = loader.ReadTextFile(full);
        if (CollectedSources) {
            CollectedSources->Texts[fixed.GetPath()] = text;
        }
        return text;
    }

    GRANET_COMPILER_CHECK(false, source, MSG_FILE_NOT_FOUND, path);
    return "";
}

TSourceText::TConstRef TSourcesLoader::ReadFileAsSourceText(const TTextView& source, const TFsPath& path) {
    const TString text = ReadFileAsString(source, path);
    return TSourceText::Create(text, path, IsCompatibilityMode, UILang);
}

// ~~~~ TLinesTreeBuilder ~~~~

TLinesTreeBuilder::TLinesTreeBuilder(const TSourceText::TConstRef& sourceText)
    : OriginalText(sourceText)
{
}

TSrcLine::TRef TLinesTreeBuilder::Build() {
    CreateMaskedText();
    CreateTree();
    SplitByColon(Root.Get());
    SplitByKindOfLineBreaks(Root.Get(), ';');
    SplitByKindOfLineBreaks(Root.Get(), '|');
    UnmaskLines(Root.Get());
    return std::move(Root);
}

void TLinesTreeBuilder::CreateMaskedText() {
    TStringBuilder out;
    bool isQuoted = false;
    bool isEscaped = false;
    bool isCommented = false;
    TStack<char> bracketStack;
    for (char c : OriginalText->Text) {
        if (c == '\n' && isCommented) {
            Y_ENSURE(!isQuoted);
            Y_ENSURE(bracketStack.empty());
            isCommented = false;
        }
        if (isCommented) {
            out << '#';
        } else if (isQuoted) {
            if (isEscaped) {
                isEscaped = false;
            } else if (c == '\\') {
                isEscaped = true;
            } else if (c == '\"') {
                isQuoted = false;
            }
            out << '_';
        } else if (c == '#') {
            Check(bracketStack.empty(), out.size(), MSG_COMMENTS_INSIDE_BRACKETS);
            isCommented = true;
            out << '#';
        } else if (c == '\"') {
            isQuoted = true;
            out << '_';
        } else if (LEFT_BRACKETS.Contains(c)) {
            bracketStack.push(c);
            out << '_';
        } else if (RIGHT_BRACKETS.Contains(c)) {
            const char openBracket = LEFT_BRACKETS[RIGHT_BRACKETS.find(c)];
            Check(!bracketStack.empty() && bracketStack.top() == openBracket, out.size(), MSG_OPENING_BRACKET_NOT_FOUND);
            bracketStack.pop();
            out << '_';
        } else if (!bracketStack.empty()) {
            out << '_';
        } else {
            out << c;
        }
    }
    Check(!isQuoted, out.size(), MSG_CLOSING_QUOTE_NOT_FOUND);
    Check(bracketStack.empty(), out.size(), MSG_CLOSING_BRACKET_NOT_FOUND);
    MaskedText = TSourceText::Create(out, OriginalText);
    Y_ENSURE(MaskedText->Text.length() == OriginalText->Text.length());
}

void TLinesTreeBuilder::CreateTree() {
    Root = MakeIntrusive<TSrcLine>(MaskedText);
    Root->Source.Trunc(0);
    Root->IsParent = true;

    TStack<size_t> indentStack;
    TStack<TSrcLine*> parentStack;

    indentStack.push(0);
    parentStack.push(Root.Get());

    TTextView maskedTextRest = MaskedText;
    TTextView line;
    while (maskedTextRest.ReadLine(&line)) {
        line.Trunc(line.Str().find_first_of('#')); // remove comment
        line.StripRight();
        const size_t indent = RemoveIndent(&line);
        if (line.IsEmpty()) {
            continue;
        }
        Y_ENSURE(!indentStack.empty());
        Y_ENSURE(!parentStack.empty());
        if (indent > indentStack.top()) {
            Check(!parentStack.top()->Children.empty(), line, MSG_UNEXPECTED_INDENT);
            parentStack.push(parentStack.top()->Children.back().Get());
            indentStack.push(indent);
            parentStack.top()->Children.push_back(MakeIntrusive<TSrcLine>(line));
            continue;
        }
        while (indent < indentStack.top()) {
            indentStack.pop();
            parentStack.pop();
            Y_ENSURE(!indentStack.empty());
            Y_ENSURE(!parentStack.empty());
        }
        Check(indent == indentStack.top(), line, MSG_UNEXPECTED_INDENT);
        parentStack.top()->Children.push_back(MakeIntrusive<TSrcLine>(line));
    }
}

size_t TLinesTreeBuilder::RemoveIndent(TTextView* line) const {
    Y_ENSURE(line);
    size_t indent = 0;
    while (line->Str().StartsWith(' ')) {
        line->Skip(1);
        indent++;
    }
    Check(!line->Str().StartsWith('\t'), *line, MSG_TAB_SYMBOL_IN_INDENT);
    return indent;
}

void TLinesTreeBuilder::SplitByColon(TSrcLine* line) const {
    Y_ENSURE(line);

    const size_t pos = line->Str().find_first_of(':');
    if (pos != TString::npos) {
        line->IsParent = true;
        const TTextView before = line->Source.Head(pos).Stripped();
        const TTextView after = line->Source.Tail(pos + 1).Stripped();
        Check(!before.IsEmpty(), line->Source, MSG_EMPTY_KEY);
        line->Source = before;
        if (!after.IsEmpty()) {
            line->Children.insert(line->Children.begin(), MakeIntrusive<TSrcLine>(after));
        }
    } else if (line != Root.Get() && !line->Children.empty()) {
        Check(false, line->Children[0]->Source, MSG_UNEXPECTED_INDENT);
    }

    for (const TSrcLine::TRef& child : line->Children) {
        SplitByColon(child.Get());
    }
}

// Symbols '|' and ';' at top level (outside any brackets) treated as line breaks with same indent.
void TLinesTreeBuilder::SplitByKindOfLineBreaks(TSrcLine* line, char lineBreak) const {
    Y_ENSURE(line);

    for (size_t i = 0; i < line->Children.size(); i++) {
        TSrcLine* child = line->Children[i].Get();

        const size_t pos = child->Str().find_first_of(lineBreak);
        if (pos == TString::npos) {
            continue;
        }
        const TTextView before = child->Source.Head(pos).Stripped();
        const TTextView after = child->Source.Tail(pos + 1).Stripped();
        Check(!before.IsEmpty(), child->Source, MSG_IMPLICIT_EMPTY_TOKEN_IS_FORBIDEN);
        child->Source = before;
        if (!after.IsEmpty()) {
            line->Children.insert(line->Children.begin() + i + 1, MakeIntrusive<TSrcLine>(after));
        }
    }

    for (const TSrcLine::TRef& child : line->Children) {
        SplitByKindOfLineBreaks(child.Get(), lineBreak);
    }
}

void TLinesTreeBuilder::UnmaskLines(TSrcLine* line) const {
    UnmaskTextView(&line->Source);
    for (const TSrcLine::TRef& child : line->Children) {
        UnmaskLines(child.Get());
    }
}

void TLinesTreeBuilder::UnmaskTextView(TTextView* view) const {
    Y_ENSURE(view);
    Y_ENSURE(view->IsDefined());
    Y_ENSURE(view->GetSourceText()->Text.length() == OriginalText->Text.length());
    view->SetSourceText(OriginalText);
}

void TLinesTreeBuilder::Check(bool condition, const TTextView& view, EMessageId messageId) const {
    if (condition) {
        return;
    }
    TTextView unmasked = view;
    UnmaskTextView(&unmasked);
    GRANET_COMPILER_CHECK(false, unmasked, messageId);
}

void TLinesTreeBuilder::Check(bool condition, size_t offset, EMessageId messageId) const {
    GRANET_COMPILER_CHECK(condition, TTextView(OriginalText, offset), messageId);
}

// ~~~~ TPreprocessor ~~~~

TPreprocessor::TPreprocessor(const TSourcesLoader& sourcesLoader)
    : SourcesLoader(sourcesLoader)
{
}

TSrcLine::TRef TPreprocessor::PreprocessFile(const TTextView& source, const TFsPath& path) {
    TSourceText::TConstRef sourceText = SourcesLoader.ReadFileAsSourceText(source, path);
    return Preprocess(sourceText);
}

TSrcLine::TRef TPreprocessor::PreprocessString(const TString& text) {
    TSourceText::TConstRef sourceText = TSourceText::Create(text, "", SourcesLoader.IsCompatibilityMode, SourcesLoader.UILang);
    return Preprocess(sourceText);
}

TSrcLine::TRef TPreprocessor::Preprocess(const TSourceText::TConstRef& sourceText) {
    TSrcLine::TRef tree = TLinesTreeBuilder(sourceText).Build();
    ProcessDirectiveInclude(tree);
    return tree;
}

// Process such lines:
//   %include path/to/file
void TPreprocessor::ProcessDirectiveInclude(const TSrcLine::TRef& tree) {
    Y_ENSURE(tree);

    // Don't use range based for loop here, because array 'lines' is growing during this loop
    TVector<TSrcLine::TRef>& lines = tree->Children;
    for (int i = 0; i < lines.ysize(); i++) {
        const TSrcLine::TRef& line = lines[i];
        TStringBuf rest = line->Str();
        if (rest.NextTok(' ') != NSyntax::NDirective::Include) {
            continue;
        }
        GRANET_COMPILER_CHECK(line->Children.empty(), *line, MSG_INVALID_DIRECTIVE);
        const TFsPath path = SafeUnquote(line->Source, rest, &NSyntax::NParamRegEx::NotQuotedPath);
        TSourceText::TConstRef sourceText = SourcesLoader.ReadFileAsSourceText(line->Source, path);
        TSrcLine::TRef fileTree = TLinesTreeBuilder(sourceText).Build();
        TVector<TSrcLine::TRef>& newLines = fileTree->Children;

        lines.erase(lines.begin() + i);
        lines.insert(lines.begin() + i, newLines.begin(), newLines.end());
        i--;
    }

    for (const TSrcLine::TRef& line : lines) {
        if (!line->Children.empty()) {
            ProcessDirectiveInclude(line);
        }
    }
}

} // namespace NGranet::NCompiler
