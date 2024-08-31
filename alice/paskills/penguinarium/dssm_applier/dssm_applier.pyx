from util.generic.hash_set cimport THashSet
from util.generic.ptr cimport TIntrusivePtr
from util.generic.string cimport TString
from util.generic.vector cimport TVector
from libcpp cimport bool
from cython.operator cimport dereference as deref, preincrement as inc


cdef extern from "contrib/libs/intel/mkl/include/mkl.h" nogil:
    cdef int mkl_set_num_threads_local(int nt) except +



cdef extern from "ml/dssm/lib/states.h":
    cdef cppclass TVectorView[TYPE]:
        TVectorView(TYPE* t)
        TYPE& operator[](size_t i)
        TYPE* data()

    cdef cppclass TMatrix "NDssm3::TMatrix" [TYPE]:
        TMatrix(TYPE* t)
        TMatrix()
        TVectorView[TYPE] operator[](size_t idx)
        size_t GetNumRows()
        size_t GetNumColumns()


cdef extern from "ml/dssm/dssm/lib/graph/model.h":
    cdef cppclass TModelParameters[TYPE]:
        NDssm3_TSampleProcessor SampleProcessor


cdef extern from "ml/dssm/dssm/lib/data/data.h" nogil:
    cdef cppclass NDssm3_TSample "NDssm3::TSample":
        pass

    cdef cppclass NDssm3_TSampleProcessor "NDssm3::TSampleProcessor":
        void Sample(const TVector[TString] &fields, NDssm3_TSample* resPtr) const
        TVector[TString] QueryFields
        TVector[TString] DocFields
        TVector[TString] PlaceFields

    cdef NDssm3_TSampleProcessor NDssm3_GetSampleProcessor "NDssm3::GetSampleProcessor"(
        const TVector[TString]& queryFields,
        const TVector[TString]& docFields,
        const TVector[TString]& placeFields,
        const TVector[TString]& inputDescription,
        const TVector[TString]* auxFields,
        bool checkErrors
    ) except +


cdef extern from "util/generic/hash.h":
    cdef cppclass __yhashtable_const_iterator[TYPE]:
        const TYPE& operator*() const
        const __yhashtable_const_iterator[TYPE]& operator++() const
        const __yhashtable_const_iterator[TYPE]& operator--() const
        bool operator==(const __yhashtable_const_iterator[TYPE]& it) const
        bool operator!=(const __yhashtable_const_iterator[TYPE]& it) const


cdef extern from "ml/dssm/dssm/lib/dssm.h" namespace "NDssm3" nogil:
    cdef cppclass NDssm3_IModel "NDssm3::IModel" [TYPE]:
        NDssm3_IModel(TYPE* t)
        TVector[const TMatrix[TYPE]*] ApplyFprop(const TVector[NDssm3_TSample]& batch, const TVector[TString]& outputVariables) except +
        THashSet[TString] GetRequredInputFields(const TVector[TString]& outputVariables) except +

    cdef cppclass TModelFactory[TYPE]:
        TModelFactory(TYPE* t)
        @staticmethod
        TIntrusivePtr[TModelFactory] Load(const TString& file) except +
        TIntrusivePtr[NDssm3_IModel[TYPE]] CreateModel() except +
        TVector[TIntrusivePtr[TModelParameters[TYPE]]] Subparams


ctypedef const TMatrix[float]* TMatrixPtr


cdef _to_py_str(const TString& string):
    return bytes(string.c_str()[:string.length()])


cdef class FieldsExtractor(object):
    cdef TVector[TString] required_fields

    def __cinit__(self, const TVector[TString]& required_fields):
        self.required_fields = required_fields

    cdef extract(self, dictionary, TVector[TString]* out):
        """
        :param dictionary: <bytes, bytes|int>
        :param out: vector where to put extracted fields
        :warning: clears out `out` vector even in case of error
        """
        out.clear()
        for field in self.required_fields:
            out.push_back(self._extract_field_value(dictionary, field))

    cdef _extract_field_value(self, dictionary, const TString& field):
        """
        dssm requires string field values, but here we also allow int
        """
        field_str = _to_py_str(field)

        if field_str not in dictionary:
            raise KeyError('Field `{}` is required, but not provided'.format(field_str))
        field_value = dictionary[field_str]

        if isinstance(field_value, int):
            field_value = b'%d' % field_value

        if not isinstance(field_value, bytes):
            raise TypeError('Value of field `{}` is neither bytes nor int: `{}`'.format(field, field_value))
        return field_value


cdef class DssmApplier(object):
    """
    Python interface to DSSM applying methods.
    """
    cdef TIntrusivePtr[NDssm3_IModel[float]] dssm_model
    cdef TIntrusivePtr[TModelFactory[float]] dssm_model_factory

    def __init__(self, model_path):
        """
        :param: model_path: (bytes) path to dssm model binary
        """
        if not isinstance(model_path, (bytes, str)):
            raise ValueError("Please provide model_path of type str")
        self._load_model(model_path)
        mkl_set_num_threads_local(1)

    cdef _load_model(self, const TString& model_path):
        self.dssm_model_factory = TModelFactory[float].Load(model_path)
        self.dssm_model = self.dssm_model_factory.Get().CreateModel()

    cdef NDssm3_TSampleProcessor _get_sample_processor(self, const TVector[TString]& fields):
        return NDssm3_GetSampleProcessor(
            self.dssm_model_factory.Get().Subparams[0].Get().SampleProcessor.QueryFields,
            self.dssm_model_factory.Get().Subparams[0].Get().SampleProcessor.DocFields,
            self.dssm_model_factory.Get().Subparams[0].Get().SampleProcessor.PlaceFields,
            fields, NULL, False
        )

    cdef TVector[TString] _get_required_fields(self, const TVector[TString]& output_variables):
        cdef THashSet[TString] required_fields = self.dssm_model.Get().GetRequredInputFields(output_variables)
        cdef TVector[TString] required_fields_vec
        cdef THashSet[TString].iterator it = required_fields.begin()
        while it != required_fields.end():
            required_fields_vec.push_back(deref(it))
            inc(it)
        return required_fields_vec

    cdef _predict_batch(self, records, output_variables):
        cdef TVector[TString] outputVariables = output_variables
        cdef TVector[TString] required_fields = self._get_required_fields(outputVariables)

        cdef NDssm3_TSample sample
        cdef TVector[NDssm3_TSample] batch
        cdef TVector[TString] extracted_values
        cdef TVector[TMatrixPtr] res_matrices

        fields_extractor = FieldsExtractor(required_fields)
        sample_processor = self._get_sample_processor(required_fields)

        for record in records:
            fields_extractor.extract(record, &extracted_values)
            sample_processor.Sample(extracted_values, &sample)
            batch.push_back(sample)

        with nogil:
            res_matrices = self.dssm_model.Get().ApplyFprop(batch, outputVariables)

        res = dict()
        for idx, variable in enumerate(output_variables):
            n_rows = <long>res_matrices[idx][0].GetNumRows()
            n_cols = <long>res_matrices[idx][0].GetNumColumns()

            if n_cols == 1:
                res[variable] = [res_matrices[idx][0][i][0] for i in xrange(int(n_rows))]
            else:
                res[variable] = [[res_matrices[idx][0][i][j] for j in xrange(n_cols)] for i in xrange(n_rows)]
        return res

    def predict(self, record_s, output_variable='joint_output'):
        """
        :param record_s: dict or list of dicts
        :param output_variable: bytes | [ bytes ] model's outputs, e.g. output, joint_output, query_embedding,
                                doc_embedding [default: joint_output]
        :return float | [float] | [[float]] | dict[bytes,float] | dict[bytes,[float]] | dict[bytes,[[float]]]
        """
        if isinstance(output_variable, bytes):
            if isinstance(record_s, list) and all(isinstance(record, dict) for record in record_s):
                return self._predict_batch(record_s, [output_variable])[output_variable]
            elif isinstance(record_s, dict):
                return self._predict_batch([record_s], [output_variable])[output_variable][0]
            else:
                raise TypeError('`record_s` should be dict or list of dicts')
        elif isinstance(output_variable, list) and all(isinstance(el, bytes) for el in output_variable):
            if isinstance(record_s, list) and all(isinstance(record, dict) for record in record_s):
                return self._predict_batch(record_s, output_variable)
            elif isinstance(record_s, dict):
                per_variable = self._predict_batch([record_s], output_variable)
                return {var: batch[0] for var, batch in per_variable.iteritems()}
            else:
                raise TypeError('`record_s` should be dict or list of dicts')
        else:
            raise TypeError("output_variable should be bytes or list of bytes!")
