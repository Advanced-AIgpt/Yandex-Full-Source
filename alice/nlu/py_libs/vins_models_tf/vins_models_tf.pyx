import cPickle as pickle
import os
import numpy as np
import scipy

from collections import namedtuple
from itertools import izip

from util.generic.string cimport TString
from util.generic.hash cimport THashMap
from util.generic.vector cimport TVector
from util.system.types cimport i32
from util.system.types cimport ui32

from cython.operator cimport dereference
from libcpp cimport bool

ctypedef TVector[long long] vector_long_long


# TF MODEL CONVERTER
cdef extern from "alice/nlu/libs/tf_model_converter/tf_model_converter.h":
    cdef void ConvertModelToMemmapped(
        const TString& pbModelFileName,
        size_t minConversionSizeBytes
    )


def convert_model_to_memmapped(pb_model_file_name, min_conversion_size_bytes):
    ConvertModelToMemmapped(pb_model_file_name, min_conversion_size_bytes)

def save_serialized_model_as_memmapped(model_file_name, serialized_model, min_conversion_size_bytes):
    assert model_file_name.endswith('.mmap'), 'model name must end with .mmap'
    pb_model_file_name = model_file_name[:-4] + 'pb'
    with open(pb_model_file_name, 'wb') as f:
        f.write(serialized_model)
    convert_model_to_memmapped(pb_model_file_name, min_conversion_size_bytes)
    os.remove(pb_model_file_name)

# WORD NN CLASS_TO_INDICES SAVER
cdef extern from "alice/nlu/libs/encoder/word_nn.h" namespace "NVins":
    cdef void SaveWordNnClassToIndicesMapping(
        const TString& fileName,
        const THashMap[TString, TVector[ui32]]& mapping
    )

def save_word_nn_class_to_indices(file_name, class_to_indices):
    cdef THashMap[TString, TVector[ui32]] mapping
    for cls, indices in class_to_indices.items():
        mapping[cls].reserve(len(indices))
        for index in indices:
            mapping[cls].push_back(index)
    SaveWordNnClassToIndicesMapping(file_name, mapping)


# TF SESSION CONFIG
cdef extern from "alice/nlu/libs/tf_nn_model/tf_nn_model.h" namespace "NVins":
    cdef cppclass TSessionConfig:
        size_t NumInterOpThreads
        size_t NumIntraOpThreads
        bool UsePerSessionThreads

cdef TSessionConfig _get_session_config():
    cdef TSessionConfig config
    config.NumInterOpThreads = 1
    config.NumIntraOpThreads = 1
    config.UsePerSessionThreads = True
    return config


# METRIC LEARNING ENCODER and DSSM ENCODER
cdef extern from "alice/nlu/libs/encoder/encoder.h" namespace "NVins":
    cdef cppclass TEncoderDescription:
        THashMap[TString, TVector[TString]] InputsMapping
        TString Output

    cdef cppclass TSparseMatrix:
        TVector[vector_long_long] Indices
        TVector[i32] Values

    cdef cppclass TEncoderInput:
        TVector[i32] Lengths
        TVector[TVector[float]] Dense
        TVector[TVector[TVector[float]]] DenseSeqIds
        TVector[TVector[TVector[float]]] DenseSeq

        TSparseMatrix SparseSeqIds
        TSparseMatrix SparseIds

        TVector[TVector[i32]] WordIds
        TVector[i32] NumWords
        TVector[i32] TrigramBatchMap
        TSparseMatrix TrigramIds

    cdef cppclass TEncoder:
        TEncoder(const TString& modelDirName) except +
        TVector[TVector[float]] Encode(const TEncoderInput& data) except +
        bool UsesFeature(const TString& featureName) except +
        void Save(const TString& modelDirName) except +
        void EstablishSessionIfNotYet(const TSessionConfig& config) except +

    cdef void SaveEncoderDescription(
        const TString& modelDirName,
        const TEncoderDescription& encoderDescription
    ) except +

def save_encoder_description(model_dir_name, encoder_description):
    cdef TEncoderDescription modelDescription
    modelDescription = _convert_encoder_description(encoder_description)
    SaveEncoderDescription(model_dir_name, modelDescription)

cdef TEncoderDescription _convert_encoder_description(encoder_description):
    cdef TEncoderDescription modelDescription

    for feature, inputs in encoder_description[0].iteritems():
        modelDescription.InputsMapping[feature] = TVector[TString]()
        if isinstance(inputs, str):
            modelDescription.InputsMapping[feature].push_back(inputs)
        else:
            for input in inputs:
                modelDescription.InputsMapping[feature].push_back(input)

    modelDescription.Output = encoder_description[1]

    return modelDescription


cdef _convert_dim1_array_long_long(data, vector_long_long& result):
    for dim0 in range(len(data)):
        result.push_back(data[dim0])

cdef _convert_dim1_array_i32(data, TVector[i32]& result):
    for dim0 in range(len(data)):
        result.push_back(data[dim0])

cdef _convert_dim2_array_i32(data, TVector[TVector[i32]]& result):
    for dim0 in range(len(data)):
        result.push_back(TVector[i32]())
        for dim1 in range(len(data[dim0])):
            result[dim0].push_back(data[dim0][dim1])

cdef _convert_dim2_array_long_long(data, TVector[vector_long_long]& result):
    for dim0 in range(len(data)):
        result.push_back(vector_long_long())
        for dim1 in range(len(data[dim0])):
            result[dim0].push_back(data[dim0][dim1])

cdef _convert_dim2_array_float(data, TVector[TVector[float]]& result):
    for dim0 in range(len(data)):
        result.push_back(TVector[float]())
        for dim1 in range(len(data[dim0])):
            result[dim0].push_back(data[dim0][dim1])

cdef _convert_dim3_array_float(data, TVector[TVector[TVector[float]]]& result):
    for dim0 in range(len(data)):
        result.push_back(TVector[TVector[float]]())
        for dim1 in range(len(data[dim0])):
            result[dim0].push_back(TVector[float]())
            for dim2 in range(len(data[dim0][dim1])):
                result[dim0][dim1].push_back(data[dim0][dim1][dim2])

cdef _convert_sparse_matrix(data, TSparseMatrix& result):
    _convert_dim2_array_long_long(data.indices, result.Indices)
    _convert_dim1_array_i32(data.values, result.Values)


cdef class TfEncoder:
    cdef TEncoder* __model

    def __cinit__(self, model_dir_name):
        self.__model = new TEncoder(model_dir_name)

    def __dealloc__(self):
        del self.__model

    def encode(self, data):
        dereference(self.__model).EstablishSessionIfNotYet(_get_session_config())
        cdef TEncoderInput convertedData
        self._convert_encoder_input(data, convertedData)
        cdef TVector[TVector[float]] encodedData
        encodedData = dereference(self.__model).Encode(convertedData)
        return np.array([np.array([value for value in row]) for row in encodedData])

    def save(self, model_dir_name):
        dereference(self.__model).Save(model_dir_name)

    cdef _convert_encoder_input(self, data, TEncoderInput& encoderInput):
        if dereference(self.__model).UsesFeature('lengths'):
            _convert_dim1_array_i32(data['lengths'], encoderInput.Lengths)

        if dereference(self.__model).UsesFeature('dense'):
            _convert_dim2_array_float(data['dense'], encoderInput.Dense)

        if dereference(self.__model).UsesFeature('dense_seq'):
            _convert_dim3_array_float(data['dense_seq'], encoderInput.DenseSeq)

        if dereference(self.__model).UsesFeature('sparse_seq_ids'):
            _convert_sparse_matrix(data['sparse_seq_ids'], encoderInput.SparseSeqIds)

        if dereference(self.__model).UsesFeature('sparse_ids'):
            _convert_sparse_matrix(data['sparse_ids'], encoderInput.SparseIds)

        if dereference(self.__model).UsesFeature('dense_seq_ids_apply'):
            _convert_dim3_array_float(data['dense_seq_ids_apply'], encoderInput.DenseSeqIds)

        if dereference(self.__model).UsesFeature('word_ids'):
            _convert_dim2_array_i32(data['word_ids'], encoderInput.WordIds)

        if dereference(self.__model).UsesFeature('num_words'):
            _convert_dim1_array_i32(data['num_words'], encoderInput.NumWords)

        if dereference(self.__model).UsesFeature('trigram_batch_map'):
            _convert_dim1_array_i32(data['trigram_batch_map'], encoderInput.TrigramBatchMap)

        if dereference(self.__model).UsesFeature('trigram_ids'):
            _convert_sparse_matrix(data['trigram_ids'], encoderInput.TrigramIds)


# NN CLASSIFIER
cdef extern from "alice/nlu/libs/nn_classifier/nn_classifier.h" namespace "NVins":
    cdef cppclass TNnClassifierModelDescription:
        size_t BatchSize
        size_t PaddingLength
        TString InputNode
        TString OutputNode
        TVector[TString] LearningSwitchNodes

    cdef void SaveNnClassifierModelDescription(
        const TString& modelDirName,
        const TNnClassifierModelDescription& modelDescription
    ) except +

    cdef cppclass TNnClassifier:
        TNnClassifier(const TString& modelDirName) except +
        void Save(const TString& modelDirName) except +
        void EstablishSessionIfNotYet(const TSessionConfig& config) except +

        TVector[TVector[double]] PredictProba(const TVector[TVector[TVector[float]]]& data) except +
        TVector[TVector[double]] PredictProba(const TVector[TVector[i32]]& data) except +

        TVector[size_t] PredictFromProba(const TVector[TVector[double]]& data) except +

def save_nn_classifier_model_description(model_dir_name, model_features):
    cdef TNnClassifierModelDescription modelDescription
    modelDescription = _convert_nn_classifier_model_description(model_features)
    SaveNnClassifierModelDescription(model_dir_name, modelDescription)

cdef TNnClassifierModelDescription _convert_nn_classifier_model_description(model_description):
    cdef TNnClassifierModelDescription modelDescription

    modelDescription.BatchSize = model_description[0]
    modelDescription.PaddingLength = model_description[1]
    modelDescription.InputNode = model_description[2]
    modelDescription.OutputNode = model_description[3]
    for node_name in model_description[4]:
        modelDescription.LearningSwitchNodes.push_back(node_name)

    return modelDescription

cdef TVector[TVector[TVector[float]]] _convert_np_array_sparse_float(data):
    data = [sample.toarray() for sample in data]
    cdef TVector[TVector[TVector[float]]] result
    for sample in data:
        result.push_back(TVector[TVector[float]]())
        for dim0 in range(sample.shape[0]):
            result.back().push_back(TVector[float]())
            for dim1 in range(sample.shape[1]):
                result.back().back().push_back(sample[dim0, dim1])

    return result

cdef TVector[TVector[i32]] _convert_np_array_vector_int32(data):
    cdef TVector[TVector[i32]] result
    for sample in data:
        result.push_back(TVector[i32]())
        for value in sample:
            result.back().push_back(value)

    return result

cdef class TfNnClassifier:
    cdef TNnClassifier* __model

    def __cinit__(self, model_dir_name):
        self.__model = new TNnClassifier(model_dir_name)

    def predict_proba(self, data):
        cdef TVector[TVector[double]] predictions = self._predict_proba(data)
        return self._convert_proba(predictions)

    def predict(self, data):
        cdef TVector[size_t] predictions = self._predict(data)
        return self._convert_predictions(predictions)

    cdef TVector[TVector[double]] _predict_proba(self, data):
        assert isinstance(data, list), 'Invalid data type'
        if not len(data):
            return TVector[TVector[double]]()

        dereference(self.__model).EstablishSessionIfNotYet(_get_session_config())
        if isinstance(data[0], scipy.sparse.csr.csr_matrix):
            return dereference(self.__model).PredictProba(_convert_np_array_sparse_float(data))
        elif isinstance(data[0], list) and isinstance(data[0][0], int): # TODO(smirnovpavel): first element always exists?
            return dereference(self.__model).PredictProba(_convert_np_array_vector_int32(data))
        else:
            raise ValueError('Invalid data type')

    cdef TVector[size_t] _predict(self, data):
        return dereference(self.__model).PredictFromProba(self._predict_proba(data))

    def _convert_proba(self, const TVector[TVector[double]]& proba):
        return [[value for value in row] for row in proba]

    def _convert_predictions(self, const TVector[size_t]& predictions):
        return [value for value in predictions]

    def save(self, model_dir_name):
        dereference(self.__model).Save(model_dir_name)

    def __dealloc__(self):
        del self.__model


# RNN TAGGER
cdef extern from "alice/nlu/libs/rnn_tagger/rnn_tagger.h" namespace "NVins":
    cdef cppclass TPrediction:
        TVector[TString] Tags
        TVector[double] Probabilities
        double FullProbability
        double ClassProbability
        bool IsFromThisClass
        TPrediction() except +

    cdef cppclass TSparseFeature:
        TString Value
        float Weight
        TSparseFeature() except +
        TSparseFeature(const TString& value, float weight)

    cdef cppclass TSample: # TODO(smirnovpavel): add text and tokens
        TVector[TString] Tags

    cdef cppclass TSampleFeatures:
        TSample Sample
        THashMap[TString, TVector[TVector[float]]] DenseSeq
        THashMap[TString, TVector[TVector[TSparseFeature]]] SparseSeq

    cdef cppclass TModelDescription:
        TVector[TString] UsedDenseFeatures
        TVector[TString] UsedSparseFeatures
        size_t DecoderDimension
        size_t NumLabels
        THashMap[TString, TString] InputsMapping
        THashMap[TString, TString] OutputsMapping
        THashMap[TString, THashMap[TString, size_t]] FeaturesMapping

    cdef cppclass TRnnTaggerBase:
        TRnnTaggerBase(const TString& modelDirName) except +
        bool UsesSparseFeature(const TString& featureName) except +
        bool UsesDenseFeature(const TString& featureName) except +
        void Save(const TString& modelDirName) except +
        bool IsSessionEstablished() except +
        void EstablishSession(const TSessionConfig& config) except +

    cdef cppclass TRnnTagger(TRnnTaggerBase):
        TRnnTagger(const TString& modelDirName) except +
        TVector[TVector[TPrediction]] PredictTopForBatch(const TVector[TSampleFeatures]& data,
                                                         size_t topSize, size_t beamWidth) except +

    cdef cppclass TClassifyingRnnTagger(TRnnTagger):
        TClassifyingRnnTagger(const TString& modelDirName) except +

    cdef cppclass TClassifyingSimpleRnnTagger(TRnnTaggerBase):
        TClassifyingSimpleRnnTagger(const TString& modelDirName) except +
        TPrediction Predict(const TSampleFeatures& data) except +

    cdef void SaveModelDescription(
        const TString& modelDirName,
        const TModelDescription& modelDescription
    ) except +


cdef TModelDescription _get_rnn_tagger_model_description(model_features):
    cdef TModelDescription modelDescription

    used_dense_features = model_features[0]
    used_sparse_features = model_features[1]
    features_mapping = model_features[2]
    modelDescription.DecoderDimension = model_features[3]
    modelDescription.NumLabels = model_features[4]
    inputs_mapping = model_features[5]
    outputs_mapping = model_features[6]

    for elem in used_dense_features:
        modelDescription.UsedDenseFeatures.push_back(elem)

    for elem in used_sparse_features:
        modelDescription.UsedSparseFeatures.push_back(elem)

    cdef THashMap[TString, size_t] cppHashMap
    for feature, hash_map in features_mapping.items():
        cppHashMap.clear()
        for key, value in hash_map.items():
            cppHashMap[key] = value
        modelDescription.FeaturesMapping[feature] = cppHashMap

    for key, value in inputs_mapping.items():
        modelDescription.InputsMapping[key] = value

    for key, value in outputs_mapping.items():
        modelDescription.OutputsMapping[key] = value

    return modelDescription


def convert_model_features(model_dir_name):
    with open(os.path.join(model_dir_name, 'model_features.pkl'), 'r') as mf:
        model_features = pickle.load(mf)
    cdef TModelDescription modelDescription
    modelDescription = _get_rnn_tagger_model_description(model_features)
    SaveModelDescription(model_dir_name, modelDescription)


cdef TSampleFeatures _convert_data(TRnnTaggerBase* tagger, data):
    cdef TSampleFeatures cppData

    for word in data.sample.tags:
        cppData.Sample.Tags.push_back(word)

    cdef TVector[float] cppFloatVector
    cdef TVector[TVector[float]] cppFloatMatrix

    for key, matrix in data.dense_seq.items():
        if dereference(tagger).UsesDenseFeature(key):
            cppFloatMatrix.clear()
            for vector in matrix:
                cppFloatVector.clear()
                for value in vector:
                    cppFloatVector.push_back(value)
                cppFloatMatrix.push_back(cppFloatVector)
            cppData.DenseSeq[key] = cppFloatMatrix

    cdef TSparseFeature cppSparseFeature
    cdef TVector[TSparseFeature] cppSparseFeatureVector
    cdef TVector[TVector[TSparseFeature]] cppSparseFeatureMatrix

    for key, matrix in data.sparse_seq.items():
        if dereference(tagger).UsesSparseFeature(key):
            cppSparseFeatureMatrix.clear()
            for vector in matrix:
                cppSparseFeatureVector.clear()
                for value in vector:
                    cppSparseFeature.Value = value.value
                    cppSparseFeature.Weight = value.weight
                    cppSparseFeatureVector.push_back(cppSparseFeature)
                cppSparseFeatureMatrix.push_back(cppSparseFeatureVector)
            cppData.SparseSeq[key] = cppSparseFeatureMatrix

    return cppData


cdef class TfRnnTagger:
    cdef TRnnTagger* __tagger

    def __cinit__(self, model_dir_name):
        self.__tagger = new TRnnTagger(model_dir_name)

    def __dealloc__(self):
        del self.__tagger

    def predict(self, batch, top_size, beam_width):
        if not dereference(self.__tagger).IsSessionEstablished():
            dereference(self.__tagger).EstablishSession(_get_session_config())

        cdef TVector[TSampleFeatures] cppDataset
        for data in batch:
            cppDataset.push_back(_convert_data(self.__tagger, data))
        cdef TVector[TVector[TPrediction]] cppPredictions
        cppPredictions = dereference(self.__tagger).PredictTopForBatch(cppDataset, top_size, beam_width)
        return self._convert_predictions(cppPredictions)

    def save(self, model_dir_name):
        dereference(self.__tagger).Save(model_dir_name)

    cdef _convert_predictions(self, TVector[TVector[TPrediction]] cppPredictions):
        out_tags, out_scores = [], []
        for predForSample in cppPredictions:
            nbest, scores = [], []
            for pred in predForSample:
                tags = []
                for tag in pred.Tags:
                    tags.append(unicode(tag))
                nbest.append(tags)
                scores.append(pred.FullProbability)
            out_tags.append(nbest)
            out_scores.append(scores)

        return out_tags, out_scores


TfClassifyingRnnTaggerPrediction = namedtuple(
    'TfClassifyingRnnTaggerPrediction',
    'tags tag_probs full_probability class_probability is_from_this_class'
)


cdef _convert_classifying_prediction(TPrediction cppPrediction):
    tags, tag_probs = [], []
    for tag, prob in izip(cppPrediction.Tags, cppPrediction.Probabilities):
        tags.append(unicode(tag))
        tag_probs.append(prob)

    return TfClassifyingRnnTaggerPrediction(
        tags=tags,
        tag_probs=tag_probs,
        full_probability=cppPrediction.FullProbability,
        class_probability=cppPrediction.ClassProbability,
        is_from_this_class=cppPrediction.IsFromThisClass
    )


cdef class TfClassifyingRnnTagger:
    cdef TClassifyingRnnTagger* __tagger

    def __cinit__(self, model_dir_name):
        self.__tagger = new TClassifyingRnnTagger(model_dir_name)

    def __dealloc__(self):
        del self.__tagger

    def predict(self, batch, top_size, beam_width):
        if not dereference(self.__tagger).IsSessionEstablished():
            dereference(self.__tagger).EstablishSession(_get_session_config())

        cdef TVector[TSampleFeatures] cppDataset
        for data in batch:
            cppDataset.push_back(_convert_data(self.__tagger, data))

        cdef TVector[TVector[TPrediction]] cppPredictions
        cppPredictions = dereference(self.__tagger).PredictTopForBatch(cppDataset, top_size, beam_width)

        return [
            [_convert_classifying_prediction(cppPrediction) for cppPrediction in samplePrediction]
            for samplePrediction in cppPredictions
        ]

    def save(self, model_dir_name):
        dereference(self.__tagger).Save(model_dir_name)


cdef class TfClassifyingSimpleRnnTagger:
    cdef TClassifyingSimpleRnnTagger* __tagger

    def __cinit__(self, model_dir_name):
        self.__tagger = new TClassifyingSimpleRnnTagger(model_dir_name)

    def __dealloc__(self):
        del self.__tagger

    def predict(self, batch):
        if not dereference(self.__tagger).IsSessionEstablished():
            dereference(self.__tagger).EstablishSession(_get_session_config())

        predictions = []
        for data in batch:
            cppPrediction = dereference(self.__tagger).Predict(_convert_data(self.__tagger, data))
            predictions.append(_convert_classifying_prediction(cppPrediction))
        return predictions

    def save(self, model_dir_name):
        dereference(self.__tagger).Save(model_dir_name)
