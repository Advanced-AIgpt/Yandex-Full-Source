#include <boost/python.hpp>
#include <boost/python/suite/indexing/vector_indexing_suite.hpp>
#include <boost/python/stl_iterator.hpp>
#include <boost/shared_ptr.hpp>
#include "crfsuitextagger.h"

namespace bp = boost::python;

typedef vector<string> std_vector_string;
typedef vector<double> std_vector_double;
typedef vector<vector<string> > std_vector_vector_string;
typedef vector<vector<double> > std_vector_vector_double;


BOOST_PYTHON_MODULE(_crfsuitex)
{
    bp::class_<std_vector_string>("std_vector_string")
            .def(bp::vector_indexing_suite<std_vector_string>());

    bp::class_<std_vector_double>("std_vector_double")
            .def(bp::vector_indexing_suite<std_vector_double>());

    bp::class_<std_vector_vector_string>("std_vector_vector_string")
            .def(bp::vector_indexing_suite<std_vector_vector_string>());

    bp::class_<std_vector_vector_double>("std_vector_vector_double")
            .def(bp::vector_indexing_suite<std_vector_vector_double>());

    bp::class_<CRFSuiteXTagger, boost::shared_ptr<CRFSuiteXTagger>, boost::noncopyable >("CRFSuiteXTagger")
            .add_property("nbest", &CRFSuiteXTagger::getNBest)
            .add_property("scores", &CRFSuiteXTagger::getScores)
            .add_property("model", &CRFSuiteXTagger::getModel, &CRFSuiteXTagger::setModel)
            .add_property("npaths", &CRFSuiteXTagger::getNPaths, &CRFSuiteXTagger::setNPaths)
            .add_property("normalized_scores", &CRFSuiteXTagger::getNormalizedScores, &CRFSuiteXTagger::setNormalizedScores)
            .def("load_from_file", &CRFSuiteXTagger::loadFromFile)
            .def("decode", &CRFSuiteXTagger::decode)
            .def("decode_nbest", &CRFSuiteXTagger::decode_nbest)
            ;
}

