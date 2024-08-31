#include "compiler.h"
#include "compiler_check.h"
#include "element_writer.h"
#include "expression_tree_builder.h"
#include "optimization_info_builder.h"
#include "preprocessor.h"
#include "skip_params_calculator.h"
#include "syntax.h"
#include <alice/nlu/granet/lib/sample/entity_utils.h>
#include <alice/nlu/granet/lib/utils/string_utils.h>
#include <alice/nlu/granet/lib/utils/utils.h>
#include <dict/nerutil/tstimer.h>
#include <library/cpp/iterator/zip.h>
#include <library/cpp/iterator/enumerate.h>
#include <util/charset/wide.h>
#include <util/generic/algorithm.h>
#include <util/generic/cast.h>
#include <util/generic/is_in.h>
#include <util/stream/file.h>
#include <util/string/ascii.h>
#include <util/string/builder.h>
#include <util/string/escape.h>
#include <util/string/join.h>
#include <util/string/split.h>
#include <util/string/strip.h>

namespace NGranet::NCompiler {

// ~~~~ TCompiler ~~~~

TCompiler::TCompiler(const TCompilerOptions& options)
    : Options(options)
{
    StringPool.Insert("");
}

TGrammar::TRef TCompiler::CompileFromPath(const TFsPath& path, const TGranetDomain& domain, IDataLoader* loader) {
    return Process(path, "", domain, loader, nullptr);
}

TGrammar::TRef TCompiler::CompileFromString(TStringBuf str, const TGranetDomain& domain, IDataLoader* loader) {
    return Process("", str, domain, loader, nullptr);
}

TSourceTextCollection TCompiler::CollectSourceTexts(const TFsPath& path, const TGranetDomain& domain, IDataLoader* loader) {
    TSourceTextCollection collection;
    collection.Domain = domain;
    // Perform both stages:
    // - ReadProject - to collect sources,
    // - CompileProject - to validate grammar.
    Process(path, "", domain, loader, &collection);
    return collection;
}

TGrammar::TRef TCompiler::CompileFromSourceTextCollection(const TSourceTextCollection& collection) {
    Data.ExternalSource = collection.ExternalSource;
    TReaderFromSourceTextCollection reader(collection);
    return CompileFromPath(collection.MainTextPath, collection.Domain, &reader);
}

TGrammar::TRef TCompiler::Process(const TFsPath& mainFilePath, TStringBuf mainText,
    const TGranetDomain& domain, IDataLoader* loader, TSourceTextCollection* collectedSources)
{
    DEBUG_TIMER("TCompiler::Compile");

    Data.Domain = domain;
    const TFsPath mainFileName = PrepareSourcesLoader(mainFilePath, loader, collectedSources);

    ReadProject(mainFileName, mainText);

    CompileTasks(Project);

    CompileElements();

    Postprocess();

    return TGrammar::Create(std::move(Data));
}

TFsPath TCompiler::PrepareSourcesLoader(const TFsPath& mainFilePath, IDataLoader* loader,
    TSourceTextCollection* collectedSources)
{
    SourcesLoader.IsCompatibilityMode = Options.IsCompatibilityMode;
    SourcesLoader.UILang = Options.UILang;
    SourcesLoader.SourceDirs = Options.SourceDirs;
    SourcesLoader.Loader = loader;
    SourcesLoader.CollectedSources = collectedSources;

    if (!mainFilePath.IsDefined()) {
        Y_ENSURE(SourcesLoader.CollectedSources == nullptr);
        return "";
    }
    TPathSplit split = mainFilePath.PathSplit();
    Y_ENSURE(!split.empty());
    const TFsPath mainFileName = split.back();
    split.pop_back();
    const TFsPath mainFileDir = split.Reconstruct();
    SourcesLoader.SourceDirs.insert(SourcesLoader.SourceDirs.begin(), mainFileDir);
    if (SourcesLoader.CollectedSources) {
        SourcesLoader.CollectedSources->MainTextPath = mainFileName.GetPath();
    }
    return mainFileName;
}

void TCompiler::ReadProject(const TFsPath& mainFileName, TStringBuf mainText) {
    DEBUG_TIMER("TCompiler::ReadProject");
    Y_ENSURE(Scopes.empty());
    Y_ENSURE(Project == nullptr);

    Project = &CreateScope(ST_Project, nullptr);
    CreateFileScope(mainFileName);

    // Don't use range based for loop here, because array Project->Children is growing during this loop
    for (size_t i = 0; i < Project->Children.size(); i++) {
        TScope* scope = Project->Children[i];

        TPreprocessor preprocessor(SourcesLoader);

        scope->Lines = (i == 0 && !scope->AsFile.IsDefined())
            ? preprocessor.PreprocessString(TString{mainText})
            : preprocessor.PreprocessFile({}, scope->AsFile);

        scope->Lines->CheckAllowedChildren(NSyntax::NFileKey::AllowedKeys);

        ReadTasks(scope);
        ReadImports(scope);
        ReadElements(scope->Lines, scope);
    }
}

TScope& TCompiler::CreateFileScope(TFsPath path) {
    if (path.IsDefined()) {
        path = path.Fix();
    }
    TScope*& scope = FileNameToScope[path.GetPath()];
    if (scope != nullptr) {
        return *scope;
    }
    const TString name = "F" + ToString(Project->Children.size());
    scope = &CreateScope(ST_File, Project);
    scope->AsFile = path;
    scope->Name = name;
    return *scope;
}

TScope& TCompiler::CreateScope(EScopeType type, TScope* parent) {
    TScope& scope = Scopes.emplace_back();
    scope.Type = type;
    scope.Parent = parent;
    if (parent != nullptr) {
        parent->Children.push_back(&scope);
    }
    return scope;
}

void TCompiler::ReadTasks(TScope* fileScope) {
    Y_ENSURE(fileScope);
    Y_ENSURE(fileScope->Lines);
    Y_ENSURE(fileScope->Type == ST_File);

    for (const TSrcLine::TRef& line : fileScope->Lines->Children) {
        TStringBuf nameBuf = line->Str();
        TStringBuf typeBuf = nameBuf.NextTok(' ');
        EParserTaskType type = PTT_COUNT;
        if (typeBuf == NSyntax::NFileKey::Form) {
            type = PTT_FORM;
        } else if (typeBuf == NSyntax::NFileKey::Entity) {
            type = PTT_ENTITY;
        } else {
            continue;
        }
        const TString name(StripString(nameBuf));
        GRANET_COMPILER_CHECK(!name.empty(), *line, MSG_NAME_IS_EMPTY);
        CheckTaskName(line, name, type);
        line->CheckAllowedChildren(NSyntax::NTaskKey::AllowedCommonKeys,
            NSyntax::NTaskKey::AllowedSpecificKeys[type]);

        GRANET_COMPILER_CHECK(!TaskNames[type].contains(name), *line, MSG_DUPLICATED_NAME, name);
        TaskNames[type].insert(name);

        TParserTask& task = Tasks[type].emplace_back();
        task.Name = name;
        task.Type = type;
        task.Index = Tasks[type].size() - 1;

        TScope& taskScope = CreateScope(ST_Task, fileScope);
        taskScope.Name = ToString<EParserTaskType>(type) + "(" + name + ")";
        taskScope.Lines = line;
        taskScope.AsTask = &task;
    }
}

void TCompiler::ReadImports(TScope* scope) {
    Y_ENSURE(scope);
    for (TScope* child : scope->Children) {
        ReadImports(child);
    }

    if (scope->Type != ST_File && scope->Type != ST_Task) {
        return;
    }
    for (const TSrcLine::TRef& line : scope->Lines->Children) {
        if (line->Str() != NSyntax::NCommonKey::Import) {
            return;
        }
        for (const TSrcLine::TRef& pathLine : line->Children) {
            const TFsPath path = SafeUnquote(pathLine->Source, &NSyntax::NParamRegEx::NotQuotedPath);
            GRANET_COMPILER_CHECK(path.IsDefined(), *pathLine, MSG_UNDEFINED_PATH);
            const TScope& imported = CreateFileScope(path);
            if (!IsIn(scope->ImportedFiles, &imported)) {
                scope->ImportedFiles.push_back(&imported);
            }
        }
    }
}

void TCompiler::ReadElements(const TSrcLine::TRef& lines, TScope* scope) {
    Y_ENSURE(scope);

    for (const TSrcLine::TRef& line : lines->Children) {
        const TStringBuf name = line->Str();

        bool isElement = false;
        if (scope->Type == ST_Element && line->IsParent) {
            isElement = true;
            CheckElementName(line, name);
        } else if (scope->Type == ST_File || scope->Type == ST_Task) {
            if (name == NSyntax::NElement::Root || name == NSyntax::NElement::Filler) {
                isElement = true;
            } else if (name.StartsWith(NSyntax::NElement::NamePrefix)) {
                isElement = true;
                CheckElementName(line, name);
            }
        }

        if (isElement) {
            TScope& child = CreateExplicitElementDefinition(line, name, scope);
            ReadElements(child.Lines, &child);
            continue;
        }

        if (scope->Type == ST_Task && name == NSyntax::NTaskKey::Values) {
            TScope& child = CreateExplicitElementDefinition(line, NSyntax::NElement::Root, scope);
            for (const TSrcLine::TRef& valueLine : line->Children) {
                ReadElements(valueLine, &child);
            }
        }
    }
    if (scope->Type == ST_File) {
        for (TScope* child : scope->Children) {
            if (child->Type == ST_Task) {
                ReadElements(child->Lines, child);
            }
        }
    }
}

void TCompiler::CheckTaskName(const TSrcLine::TRef& line, TStringBuf name, EParserTaskType type) const {
    const TRegExMatch& regEx = type == PTT_FORM
        ? (Data.Domain.IsPASkills ? NSyntax::NParamRegEx::FormNamePASkills : NSyntax::NParamRegEx::FormName)
        : NSyntax::NParamRegEx::TypeName;
    CheckName(line, name, regEx, MSG_INVALID_NAME);
}

void TCompiler::CheckTypeName(const TSrcLine::TRef& line, TStringBuf name) const {
    const TRegExMatch& regEx = Data.Domain.IsPASkills
        ? NSyntax::NParamRegEx::TypeNamePASkills : NSyntax::NParamRegEx::TypeName;
    CheckName(line, name, regEx, MSG_INVALID_TYPE_NAME);
}

void TCompiler::CheckSlotName(const TSrcLine::TRef& line, TStringBuf name) const {
    CheckName(line, name, NSyntax::NParamRegEx::SlotName, MSG_INVALID_SLOT_NAME);
}

void TCompiler::CheckElementName(const TSrcLine::TRef& line, TStringBuf name) const {
    CheckName(line, name, NSyntax::NParamRegEx::ElementName, MSG_INVALID_ELEMENT_NAME);
}

void TCompiler::CheckName(const TSrcLine::TRef& line, TStringBuf name, const TRegExMatch& regEx,
    EMessageId message) const
{
    GRANET_COMPILER_CHECK(Options.IsCompatibilityMode || regEx.Match(TString(name).c_str()), *line, message, name);
}

// ~~~~ Compile tasks ~~~~

void TCompiler::CompileTasks(TScope* scope) {
    DEBUG_TIMER("TCompiler::CompileTasks");
    Y_ENSURE(scope);

    // Recursion
    for (TScope* child : scope->Children) {
        CompileTasks(child);
    }

    // Compile current task
    TParserTask* task = scope->AsTask;
    if (task == nullptr) {
        return;
    }
    Y_ENSURE(scope->Type == ST_Task);
    Y_ENSURE(scope->Lines);

    TSrcLine::TRef lines = scope->Lines;

    task->EnableGranetParser = lines->GetBooleanValueByKey(NSyntax::NTaskKey::EnableGranetParser, true);
    task->EnableAliceTagger = lines->GetBooleanValueByKey(NSyntax::NTaskKey::EnableAliceTagger, false);
    task->EnableAutoFiller = lines->GetBooleanValueByKey(NSyntax::NTaskKey::EnableAutoFiller,
        (Data.Domain.IsPASkills || Data.Domain.IsWizard) && task->Type == PTT_FORM);
    if (const auto value = lines->FindValueByKey(NSyntax::NTaskKey::EnableSynonyms)) {
        ReadSynonyms(lines->Source, value->Str(), task->EnableSynonymFlagsMask, task->EnableSynonymFlags, true);
    }
    if (const auto value = lines->FindValueByKey(NSyntax::NTaskKey::DisableSynonyms)) {
        ReadSynonyms(lines->Source, value->Str(), task->EnableSynonymFlagsMask, task->EnableSynonymFlags, false);
    }
    task->KeepVariants = lines->GetBooleanValueByKey(NSyntax::NTaskKey::KeepVariants, false);
    task->KeepOverlapped = lines->GetBooleanValueByKey(NSyntax::NTaskKey::KeepOverlapped, false);
    task->IsAction = lines->GetBooleanValueByKey(NSyntax::NTaskKey::IsAction, false);
    task->IsFixlist = lines->GetBooleanValueByKey(NSyntax::NTaskKey::IsFixlist, false);
    task->IsConditional = lines->GetBooleanValueByKey(NSyntax::NTaskKey::IsConditional, task->IsAction);
    task->IsInternal = lines->GetBooleanValueByKey(NSyntax::NTaskKey::IsInternal, task->IsInternal);
    task->Fresh = lines->GetBooleanValueByKey(NSyntax::NTaskKey::Fresh, false);
    task->Freshness = lines->GetUnsignedIntValueByKey(NSyntax::NTaskKey::Freshness, 0);

    if (const TSrcLine* slotsLine = lines->FindKey(NSyntax::NTaskKey::Slots)) {
        for (const TSrcLine::TRef& slotLine : slotsLine->Children) {
            CompileSlot(scope, slotLine, task);
        }
    }

    if (task->EnableGranetParser) {
        TElementModification modification;
        SetFlags(&modification.CompilerFlags, CF_LEMMA_GOOD, lines->GetBooleanValueByKey(NSyntax::NTaskKey::Lemma, false));
        SetFlags(&modification.CompilerFlags, CF_LEMMA_AS_IS, lines->GetBooleanValueByKey(NSyntax::NTaskKey::LemmaAsIs, false));
        SetFlags(&modification.CompilerFlags, CF_INFLECT_CASES, lines->GetBooleanValueByKey(NSyntax::NTaskKey::InflectCases, false));
        SetFlags(&modification.CompilerFlags, CF_INFLECT_GENDERS, lines->GetBooleanValueByKey(NSyntax::NTaskKey::InflectGenders, false));
        SetFlags(&modification.CompilerFlags, CF_INFLECT_NUMBERS, lines->GetBooleanValueByKey(NSyntax::NTaskKey::InflectNumbers, false));
        task->Root = EnsureElementByName(lines->Source, NSyntax::NElement::Root, modification, scope).Id;
        task->UserFiller = GetOptionalElementId(TryEnsureElementByName(lines->Source, NSyntax::NElement::Filler, {}, scope));
        if (task->EnableAutoFiller) {
            task->AutoFiller = EnsureElementByName(lines->Source, NSyntax::NElement::AutoFiller, {}, scope).Id;
        }
    }
}

void TCompiler::CompileSlot(const TScope* scope, const TSrcLine::TRef& slotLine, TParserTask* task) {
    Y_ENSURE(scope);
    Y_ENSURE(task);

    const TString slotName(slotLine->Str());
    CheckSlotName(slotLine, slotName);
    for (const TSlotDescription& other : task->Slots) {
        GRANET_COMPILER_CHECK(slotName != other.Name, *slotLine, MSG_DUPLICATED_SLOT_NAME);
    }
    TSlotDescription& slot = task->Slots.emplace_back();
    slot.Name = slotName;

    slotLine->CheckAllowedChildren(NSyntax::NSlotKey::AllowedKeys);

    const TSrcLine* typesLine = slotLine->FindKey(NSyntax::NSlotKey::Type, NSyntax::NSlotKey::Types);
    GRANET_COMPILER_CHECK(Options.IsCompatibilityMode || Data.Domain.IsPASkills || typesLine, *slotLine, MSG_NO_TYPE_SECTION);
    if (typesLine) {
        for (const TSrcLine::TRef& typeLine : typesLine->Children) {
            const TString typeName(typeLine->Str());
            CheckTypeName(typeLine, typeName);
            slot.DataTypes.push_back(typeName);
        }
    }
    if (const TSrcLine* sourcesLine = slotLine->FindKey(NSyntax::NSlotKey::Source, NSyntax::NSlotKey::Sources)) {
        for (const TSrcLine::TRef& elementLine : sourcesLine->Children) {
            const TString elementName(elementLine->Str());
            CheckElementName(elementLine, elementName);
            TElementDefinition& element = EnsureElementDefinitionByName(elementLine->Source, elementName, scope);
            element.SourceForSlots.push_back({task->Type, task->Index, task->Slots.size() - 1});
        }
    }
    if (const TSrcLine* matchingTypeLine = slotLine->FindValueByKey(NSyntax::NSlotKey::MatchingType)) {
        const TStringBuf matchingType = matchingTypeLine->Str();
        GRANET_COMPILER_CHECK(TryFromString<ESlotMatchingType>(matchingType, slot.MatchingType),
            *matchingTypeLine, MSG_INVALID_MATCHING_TYPE, matchingType);
    }
    if (const TSrcLine* concatenateStringsLine = slotLine->FindValueByKey(NSyntax::NSlotKey::ConcatenateStrings)) {
        const TStringBuf concatenateStrings = concatenateStringsLine->Str();
        GRANET_COMPILER_CHECK(TryFromString<bool>(concatenateStrings, slot.ConcatenateStrings),
            *concatenateStringsLine, MSG_INVALID_CONCATENATE_STRINGS, concatenateStrings);
    }
    if (const TSrcLine* keepVariantsLine = slotLine->FindValueByKey(NSyntax::NSlotKey::KeepVariants)) {
        const TStringBuf keepVariants = keepVariantsLine->Str();
        GRANET_COMPILER_CHECK(TryFromString<bool>(keepVariants, slot.KeepVariants),
            *keepVariantsLine, MSG_INVALID_KEEP_VARIANTS, keepVariants);
    }
    GRANET_COMPILER_CHECK(!slot.ConcatenateStrings || task->EnableAliceTagger, *slotLine, MSG_CONCATENATE_STRINGS_SUPPORTED_ONLY_FOR_TAGGER);
}

// ~~~~ Create and search elements ~~~~

TScope& TCompiler::CreateExplicitElementDefinition(const TSrcLine::TRef& line, TStringBuf name, TScope* parent) {
    TScope& scope = CreateScope(ST_Element, parent);
    scope.Name = name;
    scope.Lines = line;

    TElementDefinition& definition = ElementDefinitions.emplace_back();
    definition.Name = name;
    definition.Scope = &scope;
    definition.Source = line->Source;
    definition.IsBuiltin = false;

    MakeExplicitElementDefinitionVisible(&definition, parent);
    return scope;
}

// static
void TCompiler::MakeExplicitElementDefinitionVisible(TElementDefinition* definition, TScope* scope) {
    Y_ENSURE(definition);
    TString fullName = definition->Name;
    for (; scope != nullptr; scope = scope->Parent) {
        const auto& [it, isNew] = scope->VisibleElements.try_emplace(fullName, definition);
        GRANET_COMPILER_CHECK(isNew, definition->Source, MSG_DUPLICATED_ELEMENT_NAME, fullName);
        if (!scope->Name.empty()) {
            TStringBuf rightPart = fullName;
            rightPart.SkipPrefix(NSyntax::NElement::NamePrefix);
            fullName = Join('.', scope->Name, rightPart);
        }
    }
}

TCompilerElement& TCompiler::EnsureElementByName(const TTextView& source, TStringBuf name,
    const TElementModification& modification, const TScope* scope)
{
    TCompilerElement* element = TryEnsureElementByName(source, name, modification, scope);
    GRANET_COMPILER_CHECK(element, source, MSG_CAN_NOT_FIND_ELEMENT, name);
    return *element;
}

TCompilerElement* TCompiler::TryEnsureElementByName(const TTextView& source, TStringBuf name,
    const TElementModification& modification, const TScope* scope)
{
    TElementDefinition* definition = TryEnsureElementDefinitionByName(source, name, scope);
    if (definition == nullptr) {
        return nullptr;
    }
    if (definition->IsBuiltin) {
        // Ignore modification for builtin elements
        Y_ENSURE(definition->Elements.size() == 1);
        return definition->Elements[0];
    }
    return &EnsureElementModification(modification, definition);
}

TElementDefinition& TCompiler::EnsureElementDefinitionByName(const TTextView& source,
    TStringBuf name, const TScope* scope)
{
    TElementDefinition* definition = TryEnsureElementDefinitionByName(source, name, scope);
    GRANET_COMPILER_CHECK(definition, source, MSG_CAN_NOT_FIND_ELEMENT, name);
    return *definition;
}

TElementDefinition* TCompiler::TryEnsureElementDefinitionByName(const TTextView& source,
    TStringBuf name, const TScope* scope)
{
    if (TElementDefinition* definition = FindExplicitElementDefinitionByName(source, name, scope)) {
        return definition;
    }
    return TryEnsureBuiltinElement(source, name);
}

TElementDefinition* TCompiler::FindExplicitElementDefinitionByName(const TTextView& source,
    TStringBuf name, const TScope* scope)
{
    while (scope != nullptr) {
        TElementDefinition* definition = scope->VisibleElements.Value(name, nullptr);
        if (definition != nullptr) {
            return definition;
        }
        // DFS in imported files
        TVector<const TScope*> dfsQueue = scope->ImportedFiles;
        for (size_t i = 0; i < dfsQueue.size(); i++) {
            Y_ENSURE(i < 1e6); // not endless loop
            const TScope* imported = dfsQueue[i];
            if (imported->VisibleElements.contains(name)) {
                GRANET_COMPILER_CHECK(definition == nullptr, source, MSG_AMBIGUOUS_ELEMENT_NAME, name);
                definition = imported->VisibleElements.at(name);
            }
            for (const TScope* next : imported->ImportedFiles) {
                if (!IsIn(dfsQueue, next)) {
                    dfsQueue.push_back(next);
                }
            }
        }
        if (definition != nullptr) {
            return definition;
        }
        scope = scope->Parent;
    }
    return nullptr;
}

TCompilerElement& TCompiler::EnsureElementModification(const TElementModification& modification, TElementDefinition* definition) {
    Y_ENSURE(definition);

    for (TCompilerElement* element : definition->Elements) {
        if (element->Modification == modification) {
            return *element;
        }
    }
    TCompilerElement& element = CreateElement(definition->Source, definition->Name, definition);
    element.Modification = modification;
    element.IsCompiled = false;
    return element;
}

TCompilerElement& TCompiler::CreateElement(const TTextView& source, TStringBuf name, TElementDefinition* definition) {
    TCompilerElement& element = Elements.emplace_back();
    element.Id = Elements.size() - 1;
    element.Name = name;
    element.Source = source;
    element.Definition = definition;
    if (definition != nullptr) {
        definition->Elements.push_back(&element);
    }
    return element;
}

TElementDefinition* TCompiler::TryEnsureBuiltinElement(const TTextView& source, TStringBuf name) {
    if (name == NSyntax::NElement::AutoFiller) {
        TElementDefinition& definition = EnsureBuiltinElementDefinition(source, name);
        if (definition.Elements.empty()) {
            TCompilerElement& element = CreateElement(source, name, &definition);
            element.IsCompiled = true;
            element.Rules.emplace_back().AppendElement(EnsureNonsenseElement(source).Id);
        }
        return &definition;
    }

    if (name == NSyntax::NElement::Void) {
        TElementDefinition& definition = EnsureBuiltinElementDefinition(source, name);
        if (definition.Elements.empty()) {
            TCompilerElement& element = CreateElement(source, name, &definition);
            element.IsCompiled = true;
            element.Rules.emplace_back();
        }
        return &definition;
    }

    const TString entityName = TryExtractEntityName(name);
    if (!entityName.empty()) {
        TElementDefinition& definition = EnsureBuiltinElementDefinition(source, name);
        if (definition.Elements.empty()) {
            TCompilerElement& element = CreateElement(source, name, &definition);
            element.IsCompiled = true;
            element.Params.EntityName = entityName;
        }
        return &definition;
    }

    return nullptr;
}

TElementDefinition& TCompiler::EnsureBuiltinElementDefinition(const TTextView& source, TStringBuf name) {
    TElementDefinition*& definition = BuiltinElements[name];
    if (definition != nullptr) {
        return *definition;
    }
    definition = &ElementDefinitions.emplace_back();
    definition->Name = name;
    definition->Scope = nullptr;
    definition->Source = source;
    definition->IsBuiltin = true;
    return *definition;
}

const TCompilerElement& TCompiler::EnsureNonsenseElement(const TTextView& source) {
    const TString name = NSyntax::NElement::NamePrefix +
        (Data.Domain.IsPASkills ? NEntityTypes::PA_SKILLS_NONSENSE : NEntityTypes::NONSENSE);
    const TElementDefinition* definition = TryEnsureBuiltinElement(source, name);
    GRANET_COMPILER_CHECK(definition, source, MSG_CAN_NOT_FIND_ELEMENT, name);
    Y_ENSURE(definition->Elements.size() == 1);
    return *definition->Elements[0];
}

TString TCompiler::TryExtractEntityName(TStringBuf name) const {
    if (!name.SkipPrefix(NSyntax::NElement::NamePrefix)) {
        return "";
    }
    if (TaskNames[PTT_ENTITY].contains(name)) {
        return TString{name};
    }
    if (Data.Domain.IsPASkills) {
        if (name.StartsWith(NEntityTypePrefixes::PA_SKILLS)) {
            return TString{name};
        }
    } else {
        // TODO(samoylovboris) keep "ner." prefix in types like GeoAddr
        if (name.SkipPrefix(TStringBuf("ner."))
            || name.SkipPrefix(TStringBuf("Ner."))
            || ALLOWED_ENTITY_GROUPS.contains(name.Before('.')))
        {
            return TString{name};
        }
    }
    return "";
}

// ~~~~ Compile elements ~~~~

void TCompiler::CompileElements() {
    // Use old-style loop because array Elements grows in iterations
    for (size_t i = 0; i < Elements.size(); i++) {
        CompileElement(&Elements[i]);
    }
}

void TCompiler::CompileElement(TCompilerElement* element) {
    Y_ENSURE(element);
    if (element->IsCompiled) {
        return;
    }

    const TElementDefinition* definition = element->Definition;
    Y_ENSURE(definition, "Builtin and subexpression elements are compiled right after creation");

    const TScope* scope = definition->Scope;
    Y_ENSURE(scope);
    Y_ENSURE(scope->Type == ST_Element);

    TSrcLine::TRef lines = scope->Lines;
    Y_ENSURE(lines);

    TParserTask* task = nullptr;
    if (scope->Parent != nullptr) {
        task = scope->Parent->AsTask;
    }
    const bool isFiller = element->Name == NSyntax::NElement::Filler;
    const bool isRoot = element->Name == NSyntax::NElement::Root;
    const bool isRootOfEntity = isRoot && task != nullptr && task->Type == PTT_ENTITY;
    const bool isRootOfForm = isRoot && !isRootOfEntity;
    const bool isListOfValues = isRootOfEntity && lines->Str() == NSyntax::NTaskKey::Values;

    TCompilerOptionsAccumulator options;
    options.CompilerFlags = element->Modification.CompilerFlags;
    options.CustomInflection = element->Modification.CustomInflection;
    SetFlags(&options.ContextFlags, DCF_IN_FILLER, isFiller);
    SetFlags(&options.ContextFlags, DCF_IN_ROOT_OF_FORM, isRootOfForm);
    SetFlags(&options.ContextFlags, DCF_IN_LIST_OF_VALUES, isListOfValues);
    SetFlags(&options.CompilerFlags, CF_ENABLE_FILLERS, !isFiller);
    SetFlags(&options.CompilerFlags, CF_ANCHOR_TO_BEGIN, isRootOfForm);
    SetFlags(&options.CompilerFlags, CF_ANCHOR_TO_END, isRootOfForm);

    TExpressionTreeBuilder builder(lines->Source, Data.Domain.Lang, SourcesLoader, &StringPool);

    if (isListOfValues) {
        Y_ENSURE(task);
        options.DataType = StringPool.Insert(task->Name);
        for (const TSrcLine::TRef& valueLine : lines->Children) {
            const TString value = SafeUnquote(valueLine->Source, &NSyntax::NParamRegEx::NotQuotedValue);
            options.DataValue = StringPool.Insert(value);
            builder.ResetOptionsAccumulator(options);
            builder.ReadLines(valueLine->Children);
        }
    } else {
        builder.ResetOptionsAccumulator(options);
        builder.ReadLines(lines->Children);
    }
    builder.Finalize();

    const ECompilerFlags& flags = builder.GetOptions().CompilerFlags;
    TCompilerElementParams& params = element->Params;
    params.AnchorToBegin = flags.HasFlags(CF_ANCHOR_TO_BEGIN);
    params.AnchorToEnd = flags.HasFlags(CF_ANCHOR_TO_END);
    params.EnableFillers = flags.HasFlags(CF_ENABLE_FILLERS);
    params.CoverFillers = flags.HasFlags(CF_COVER_FILLERS);
    params.EnableEdgeFillers = params.EnableFillers && (flags.HasFlags(CF_COVER_FILLERS) || isRootOfForm);
    params.EnableSynonymFlagsMask = builder.GetOptions().EnableSynonymFlagsMask;
    params.EnableSynonymFlags = builder.GetOptions().EnableSynonymFlags;

    TExpressionNode::TConstRef tree = builder.GetTree();
    const TRuleCtx ctx = {
        .Scope = scope,
        .Modification = element->Modification,
        .SubExpressionParams = {
            .EnableFillers = flags.HasFlags(CF_ENABLE_FILLERS),
            .EnableSynonymFlagsMask = params.EnableSynonymFlagsMask,
            .EnableSynonymFlags = params.EnableSynonymFlags,
        },
    };
    if (tree->Type == ENT_LIST) {
        element->Rules = CompileListNodeOperands(ctx, tree->AsList());
        params.Quantity = {1, 1};
    } else if (tree->Type == ENT_BAG) {
        element->Rules = CompileBagNodeOperands(ctx, tree->AsBag());
        params.Quantity = {0, Max<ui8>()};
    } else {
        Y_ENSURE(false);
    }
    element->IsCompiled = true;
}

TVector<TCompiledRule> TCompiler::CompileListNodeOperands(const TRuleCtx& ctx, const TListExpressionNode* node) {
    Y_ENSURE(node);
    TVector<TCompiledRule> rules(Reserve(node->Operands.size()));
    for (const auto& [operand, params] : Zip(node->Operands, node->Params)) {
        TCompiledRule& rule = rules.emplace_back();
        rule.Chain = CompileChainNode(ctx, operand->AsChain());
        rule.ListItemParams = params;
    }
    return rules;
}

TVector<TCompiledRule> TCompiler::CompileBagNodeOperands(const TRuleCtx& ctx, const TBagExpressionNode* node) {
    Y_ENSURE(node);
    TVector<TCompiledRule> rules(Reserve(node->Operands.size()));
    for (const auto& [operand, params] : Zip(node->Operands, node->Params)) {
        TCompiledRule& rule = rules.emplace_back();
        rule.Chain = CompileChainNode(ctx, operand->AsChain());
        rule.BagItemParams = params;
    }
    return rules;
}

TVector<TTokenId> TCompiler::CompileChainNode(const TRuleCtx& ctx, const TChainExpressionNode* node) {
    Y_ENSURE(node);
    TVector<TTokenId> chain;
    for (const TExpressionNode::TRef& operand : node->Operands) {
        chain.push_back(CompileChainItem(ctx, operand.Get()));
    }
    return chain;
}

TTokenId TCompiler::CompileChainItem(const TRuleCtx& ctx, const TExpressionNode* node) {
    Y_ENSURE(node);

    const EExpressionNodeType type = node->Type;
    if (type == ENT_WORD) {
        return CompileWordNode(node->AsWord());
    }
    if (type == ENT_ELEMENT) {
        const TString name = node->AsElement()->ElementName;
        return NTokenId::FromElementId(EnsureElementByName(node->Source, name, ctx.Modification, ctx.Scope).Id);
    }
    if (type == ENT_TAG_BEGIN) {
        return TOKEN_SLOT_BEGIN;
    }
    if (type == ENT_TAG_END) {
        return NTokenId::FromSlotMarkupStringId(StringPool.Insert(node->AsTagEnd()->Tag));
    }
    if (type == ENT_QUANTIFIER || type == ENT_LIST || type == ENT_BAG) {
        TSubExpressionElementKey data;
        data.Params = ctx.SubExpressionParams;
        if (type == ENT_QUANTIFIER) {
            data.Rules = CompileListNodeOperands(ctx, node->Operands[0]->AsList());
            data.Quantity = node->AsQuantifier()->Params;
        } else if (type == ENT_LIST) {
            data.Rules = CompileListNodeOperands(ctx, node->AsList());
            data.Quantity = {1, 1};
        } else if (type == ENT_BAG) {
            data.Rules = CompileBagNodeOperands(ctx, node->AsBag());
            data.Quantity = {0, Max<ui8>()};
        } else {
            Y_ENSURE(false);
        }
        return NTokenId::FromElementId(CreateSubExpressionElement(node->Source, std::move(data)).Id);
    }
    if (type == ENT_MODIFIER) {
        return NTokenId::FromElementId(CompileModifierNode(ctx, node).Id);
    }
    Y_ENSURE(type != ENT_CHAIN); // Chain inside chain should be normalized
    Y_ENSURE(false);
    return 0;
}

TTokenId TCompiler::CompileWordNode(const TWordExpressionNode* node) {
    Y_ENSURE(node);
    if (node->Word == TStringBuf(".")) {
        return TOKEN_WILDCARD;
    }
    return TokenPool.AddWord(node->Word, node->IsLemma);
}

TCompilerElement& TCompiler::CreateSubExpressionElement(const TTextView& source, TSubExpressionElementKey&& data) {
    TCompilerElement*& element = SubExpressionElements[data];
    if (element != nullptr) {
        return *element;
    }
    const TString name = GenerateSubExpressionElementName(data, TokenPool, Elements);
    element = &CreateElement(source, name, nullptr);
    element->Params.EnableFillers = data.Params.EnableFillers;
    element->Params.EnableSynonymFlagsMask = data.Params.EnableSynonymFlagsMask;
    element->Params.EnableSynonymFlags = data.Params.EnableSynonymFlags;
    element->Params.Quantity = data.Quantity;
    element->Rules = std::move(data.Rules);
    element->IsCompiled = true;
    return *element;
}

TCompilerElement& TCompiler::CompileModifierNode(const TRuleCtx& ctx, const TExpressionNode* node) {
    Y_ENSURE(node);
    Y_ENSURE(node->Type == ENT_MODIFIER);

    TElementModification modification = ctx.Modification;
    const TExpressionNode* nestedNode = node;
    while (nestedNode->Type == ENT_MODIFIER) {
        const TModifierExpressionNode* modifierNode = nestedNode->AsModifier();
        modification = OverrideModification(modification,
            ParseElementModification(modifierNode->Suffix, modifierNode->Source));
        Y_ENSURE(nestedNode->Operands.size() == 1);
        nestedNode = nestedNode->Operands[0].Get();
    }

    Y_ENSURE(nestedNode->Type == ENT_ELEMENT);
    const TString elementName = nestedNode->AsElement()->ElementName;

    return EnsureElementByName(node->Source, elementName, modification, ctx.Scope);
}

// ~~~~ Postprocess ~~~~

void TCompiler::Postprocess() {
    DEBUG_TIMER("TCompiler::Postprocess");

    FindElementDependencies();
    FindTaskDependencies();

    // Writes:
    //   TGrammarData::Elements
    //   TGrammarData::RuleTriePool
    //   TGrammarData::TagPool
    WriteGrammarElements(Elements, &Data);

    Data.Entities = ToVector<TParserTask>(Tasks[PTT_ENTITY]);
    Data.Forms = ToVector<TParserTask>(Tasks[PTT_FORM]);
    Data.StringPool = StringPool.ReleasePool();
    TokenPool.BuildTrie(&Data.WordTrie);

    // Writes:
    //   TGrammarElement::CanSkip
    //   TGrammarElement::LogProbOfSkip
    //   TGrammarElement::SetOfPossibleEmptyRequiredRules
    //   TGrammarElement::LogProbOfRequiredRulesSkip
    TElementSkipParamsCalculator(&Data).Process();

    // Writes:
    //   TGrammarData::OptimizationInfo
    NOptimizer::TOptimizerBySpecificWords(&Data, TokenPool).Process();
    NOptimizer::TOptimizerByFirstWords(&Data, TokenPool).Process();
}

void TCompiler::FindElementDependencies() {
    TVector<TCompilerElement*> path;
    for (TCompilerElement& element : Elements) {
        FindElementDependencies(&element, &path);
    }
}

void TCompiler::FindElementDependencies(TCompilerElement* element, TVector<TCompilerElement*>* path) {
    Y_ENSURE(element);
    Y_ENSURE(path);

    if (IsIn(*path, element)) {
        path->push_back(element);
        ThrowRecursionError(*element, *path);
    }

    if (element->IsDependenciesDefined) {
        return;
    }

    element->IsDependenciesDefined = true;
    element->Level = 0;
    path->push_back(element);

    for (const TCompiledRule& rule : element->Rules) {
        for (const TTokenId token : rule.Chain) {
            if (!NTokenId::IsElement(token)) {
                continue;
            }
            const TElementId childId = NTokenId::ToElementId(token);
            if (element->Dependencies.contains(childId)) {
                continue;
            }
            TCompilerElement& child = Elements[childId];
            FindElementDependencies(&child, path);
            Y_ENSURE(child.IsDependenciesDefined);
            for (const TElementId childDependency : child.Dependencies) {
                element->Dependencies.insert(childDependency);
            }
            element->Dependencies.insert(childId);
            element->Level = Max(element->Level, child.Level + 1);
        }
    }

    path->pop_back();
    Data.ElementLevelCount = Max(Data.ElementLevelCount, element->Level + 1);
}

// static
void TCompiler::ThrowRecursionError(const TCompilerElement& element, const TVector<TCompilerElement*>& path) {
    TVector<TString> names;
    for (const TCompilerElement* pathElement : path) {
        names.push_back(pathElement->Name);
    }
    GRANET_COMPILER_CHECK(false, element.Source, MSG_RECURSION_DETECTED, JoinSeq(" -> ", names));
}

void TCompiler::FindTaskDependencies() {
    for (TDeque<TParserTask>& taskList : Tasks) {
        for (TParserTask& task : taskList) {
            if (task.Root != UNDEFINED_ELEMENT_ID) {
                for (const TElementId elementId : Elements[task.Root].Dependencies) {
                    const TString& entity = Elements[elementId].Params.EntityName;
                    if (entity.empty()) {
                        continue;
                    }
                    task.DependsOnEntities.push_back(entity);
                }
            }
            for (const TSlotDescription& slot : task.Slots) {
                Extend(slot.DataTypes, &task.DependsOnEntities);
            }
            SortUnique(task.DependsOnEntities);
        }
    }
}

} // namespace NGranet::NCompiler
