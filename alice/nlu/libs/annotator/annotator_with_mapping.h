#pragma once

#include "annotator.h"
#include "mapping.h"

#include <util/generic/string.h>


namespace NAnnotator {

    /**
     * @brief Class to build TAnnotatorWithMapping (see it first for more info). Usage exampe in annotator_ut.cpp.
     */
    template <class TData>
    class TAnnotatorWithMappingBuilder {
    public:
        // If lemmatizing parameter is true both patterns and annotated text are lemmatized before processing.
        explicit TAnnotatorWithMappingBuilder(bool lemmatizing = false)
            : AnnotatorBuilder(lemmatizing)
        {
        }

        void AddPattern(const TStringBuf pattern) {
            AnnotatorBuilder.AddPattern(pattern);
        }

        /**
         * @brief Finish single class building. Call it after adding all patterns for this class using AddPattern method.
         * @param data Data associated with the finished class.
         */
        void FinishClass(const TData& data) {
            MappingBuilder.AddItem(data);
            AnnotatorBuilder.FinishClass();
        }

        void Save(IOutputStream* output) const {
            ui64 annotatorSize = AnnotatorBuilder.EstimateSize();
            output->Write(&annotatorSize, sizeof(ui64));
            AnnotatorBuilder.Save(output);
            MappingBuilder.Save(output);
        }

    private:
        TAnnotatorBuilder AnnotatorBuilder;
        TIdToItemMappingBuilder<TData> MappingBuilder;
    };


    template <class TData>
    struct TComprehensiveAnnotation {
        TData ClassData;
        TVector<TOccurencePosition> OccurencePositions;
    };


    /**
     * @brief TAnnotatorWithMapping finds occurences of predefined classes of patterns in text.
     * Each class is a set of patterns with associated data.
     * In order to build TAnnotatorWithMapping use TAnnotatorWithMappingBuilder.
     * Usage exampe in annotator_ut.cpp.
     */
    template <class TData>
    class TAnnotatorWithMapping {
    public:
        explicit TAnnotatorWithMapping(const TBlob& blob)
            : DataHolder(blob)
        {
            const char* dataStart = blob.AsCharPtr();
            size_t dataSize = blob.Length();
            const ui64 annotatorSize = *reinterpret_cast<const ui64*>(dataStart);
            dataStart += sizeof(ui64);
            dataSize -= sizeof(ui64);

            Annotator = MakeHolder<TAnnotator>(TBlob::NoCopy(dataStart, annotatorSize));
            Mapping = MakeHolder<TIdToItemMapping<TData>>(TBlob::NoCopy(dataStart + annotatorSize, dataSize - annotatorSize));
        }

        /**
         * @brief Method to annotate text
         *
         * @param  Text Text to annotate.
         * @result List of unique pattern classes found in text.
         *         For each class the associated data is returned with a list of occurences (see TComprehensiveAnnotation).
         */
        TVector<TComprehensiveAnnotation<TData>> Annotate(const TStringBuf text) const {
            TVector<TComprehensiveAnnotation<TData>> comprehensiveAnnotations;

            auto annotations = Annotator->Annotate(text);
            for (auto& annotation : annotations) {
                const TClassId classId = annotation.first;
                auto&& occurencePositions = annotation.second;
                TData classData = Mapping->GetItem(classId);
                comprehensiveAnnotations.emplace_back(TComprehensiveAnnotation<TData>{classData, std::move(occurencePositions)});
            }

            return comprehensiveAnnotations;
        }

    private:
        TBlob DataHolder;
        THolder<TAnnotator> Annotator;
        THolder<TIdToItemMapping<TData>> Mapping;
    };

} // namespace NAnnotator
