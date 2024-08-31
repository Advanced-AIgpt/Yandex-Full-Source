#pragma once

#include "compiler_data.h"
#include "data_loader.h"
#include "element_modification.h"
#include "expression_tree.h"
#include "preprocessor.h"
#include "source_text_collection.h"
#include "sub_expression_element.h"
#include <alice/nlu/granet/lib/grammar/grammar.h>
#include <alice/nlu/granet/lib/grammar/token_id.h>
#include <library/cpp/langs/langs.h>
#include <library/cpp/regex/pcre/regexp.h>
#include <util/folder/path.h>
#include <util/generic/queue.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>

namespace NGranet::NCompiler {

// ~~~~ TCompilerOptions ~~~~

struct TCompilerOptions {
    bool IsCompatibilityMode = false;
    ELanguage UILang = LANG_ENG;
    TVector<TFsPath> SourceDirs;
};

// ~~~~ TCompiler ~~~~

class TCompiler : public TNonCopyable {
public:
    explicit TCompiler(const TCompilerOptions& options = {});

    // path - path to grammar source file.
    // loader - if not null use this callback to load needed source files (source fail, imported files).
    TGrammar::TRef CompileFromPath(const TFsPath& path, const TGranetDomain& domain, IDataLoader* loader = nullptr);

    // str - grammar string.
    TGrammar::TRef CompileFromString(TStringBuf str, const TGranetDomain& domain, IDataLoader* loader = nullptr);

    // Don't compile, just collect source files needed to compile grammar.
    // path - path to main source file.
    TSourceTextCollection CollectSourceTexts(const TFsPath& path, const TGranetDomain& domain, IDataLoader* loader = nullptr);

    // collection - storage of grammar source texts.
    TGrammar::TRef CompileFromSourceTextCollection(const TSourceTextCollection& collection);

private:
    struct TRuleCtx {
        const TScope* Scope = nullptr;
        TElementModification Modification;
        TSubExpressionElementParams SubExpressionParams;
    };

private:
    TGrammar::TRef Process(const TFsPath& mainFilePath, TStringBuf mainText, const TGranetDomain& domain,
        IDataLoader* loader, TSourceTextCollection* collectedSources);
    TFsPath PrepareSourcesLoader(const TFsPath& mainFilePath, IDataLoader* loader,
        TSourceTextCollection* collectedSources);
    void ReadProject(const TFsPath& mainFileName, TStringBuf mainText);
    TScope& CreateScope(EScopeType type, TScope* parent);
    TScope& CreateFileScope(TFsPath path);
    void ReadTasks(TScope* fileScope);
    void ReadImports(TScope* scope);
    void ReadElements(const TSrcLine::TRef& lines, TScope* scope);
    void CheckTaskName(const TSrcLine::TRef& line, TStringBuf name, EParserTaskType type) const;
    void CheckTypeName(const TSrcLine::TRef& line, TStringBuf name) const;
    void CheckSlotName(const TSrcLine::TRef& line, TStringBuf name) const;
    void CheckElementName(const TSrcLine::TRef& line, TStringBuf name) const;
    void CheckName(const TSrcLine::TRef& line, TStringBuf nameBuf, const TRegExMatch& regEx, EMessageId message) const;

    // ~~~~ Compile tasks ~~~~

    void CompileTasks(TScope* scope);
    void CompileSlot(const TScope* scope, const TSrcLine::TRef& slotLine, TParserTask* task);

    // ~~~~ Create and search elements ~~~~

    TScope& CreateExplicitElementDefinition(const TSrcLine::TRef& line, TStringBuf name, TScope* parent);
    static void MakeExplicitElementDefinitionVisible(TElementDefinition* definition, TScope* scope);
    TCompilerElement& EnsureElementByName(const TTextView& source, TStringBuf name, const TElementModification& modification, const TScope* scope);
    TCompilerElement* TryEnsureElementByName(const TTextView& source, TStringBuf name, const TElementModification& modification, const TScope* scope);
    TElementDefinition& EnsureElementDefinitionByName(const TTextView& source, TStringBuf name, const TScope* scope);
    TElementDefinition* TryEnsureElementDefinitionByName(const TTextView& source, TStringBuf name, const TScope* scope);
    TElementDefinition* FindExplicitElementDefinitionByName(const TTextView& source, TStringBuf name, const TScope* scope);
    TCompilerElement& EnsureElementModification(const TElementModification& modification, TElementDefinition* definition);
    TCompilerElement& CreateElement(const TTextView& source, TStringBuf name, TElementDefinition* definition);
    TElementDefinition* TryEnsureBuiltinElement(const TTextView& source, TStringBuf name);
    TElementDefinition& EnsureBuiltinElementDefinition(const TTextView& source, TStringBuf name);
    const TCompilerElement& EnsureNonsenseElement(const TTextView& source);
    TString TryExtractEntityName(TStringBuf name) const;

    // ~~~~ Compile elements ~~~~

    void CompileElements();
    void CompileElement(TCompilerElement* element);
    TVector<TCompiledRule> CompileListNodeOperands(const TRuleCtx& ctx, const TListExpressionNode* node);
    TVector<TCompiledRule> CompileBagNodeOperands(const TRuleCtx& ctx, const TBagExpressionNode* node);
    TVector<TTokenId> CompileChainNode(const TRuleCtx& ctx, const TChainExpressionNode* node);
    TTokenId CompileChainItem(const TRuleCtx& ctx, const TExpressionNode* node);
    TTokenId CompileWordNode(const TWordExpressionNode* node);
    TCompilerElement& CreateSubExpressionElement(const TTextView& source, TSubExpressionElementKey&& data);
    TCompilerElement& CompileModifierNode(const TRuleCtx& ctx, const TExpressionNode* node);

    // ~~~~ Postprocess ~~~~

    void Postprocess();
    void FindElementDependencies();
    void FindElementDependencies(TCompilerElement* element, TVector<TCompilerElement*>* path);
    static void ThrowRecursionError(const TCompilerElement& element, const TVector<TCompilerElement*>& path);
    void FindTaskDependencies();

private:
    TCompilerOptions Options;
    TSourcesLoader SourcesLoader;

    TDeque<TScope> Scopes;
    TScope* Project = nullptr;

    THashMap<TString, TScope*> FileNameToScope;

    TDeque<TParserTask> Tasks[PTT_COUNT];
    THashSet<TString> TaskNames[PTT_COUNT];

    TDeque<TElementDefinition> ElementDefinitions;

    // All created elements
    TDeque<TCompilerElement> Elements;

    THashMap<TString, TElementDefinition*> BuiltinElements;
    THashMap<TSubExpressionElementKey, TCompilerElement*> SubExpressionElements;

    TTokenPool TokenPool;
    TStringPool StringPool;

    TGrammarData Data;
};

} // namespace NGranet::NCompiler
