#include "crfsuitextagger.h"
#include <iostream>

CRFSuiteXTagger::CRFSuiteXTagger() : model(""), nPaths(1), normalizedScores(true) {}

const string CRFSuiteXTagger::getModel() { return model; }

const vector<vector<string> > CRFSuiteXTagger::getNBest() { return nBest; }

const vector<double> CRFSuiteXTagger::getScores() { return scores; }

int CRFSuiteXTagger::getNPaths() { return nPaths; }

bool CRFSuiteXTagger::getNormalizedScores() { return normalizedScores; }

void CRFSuiteXTagger::setModel(const string &Model) {
    model = Model;
    tagger.open(model.c_str(), model.size());
}

void CRFSuiteXTagger::setNPaths(int NPaths) { nPaths = NPaths; }

void CRFSuiteXTagger::setNormalizedScores(bool NormalizedScores) { normalizedScores = NormalizedScores; }

CRFSuite::ItemSequence CRFSuiteXTagger::fromVectorVector(const vector<vector<string> > &input, const vector<vector<double> > &weights)
{
    CRFSuite::ItemSequence xseq(input.size(), CRFSuite::Item());
    CRFSuite::ItemSequence::iterator xitem = xseq.begin();
    vector<vector<string> >::const_iterator input_it = input.begin();
    vector<vector<double> >::const_iterator weights_it = weights.begin();
    for (; input_it != input.end(); ++input_it, ++weights_it, ++xitem)
    {
        xitem->reserve(input_it->size());
        vector<string>::const_iterator input_it1 = input_it->begin();
        vector<double>::const_iterator weights_it1 = weights_it->begin();
        for (; input_it1 != input_it->end(); ++input_it1, ++weights_it1)
        {
            CRFSuite::Attribute x(*input_it1,*weights_it1);
            xitem->push_back(x);
        }
    }
    return xseq;
}

bool CRFSuiteXTagger::loadModelFromFile(const char *filename)
{
    FILE* f = fopen(filename, "rb");
    fseek(f, 0, SEEK_END);
    int model_size = ftell(f);
    fseek(f, 0, SEEK_SET);
    model.resize(model_size);
    fread(&model[0], model_size, 1, f);
    fclose(f);
    return true;
}

int CRFSuiteXTagger::loadFromFile(const string &FileName)
{
    if (!loadModelFromFile(FileName.c_str()))
        return -1;
    if (!tagger.open(model.c_str(), model.size()))
        return -1;
    return 0;
}

int CRFSuiteXTagger::decode(const vector<vector<string> > &input, const vector<vector<double> > &weights)
{
    CRFSuite::ItemSequence xseq = fromVectorVector(input, weights);
    tagger.set(xseq);
    double score = 0;
    CRFSuite::StringList yseq = tagger.viterbi(score, normalizedScores);
    nBest.resize(1, yseq);
    scores.resize(1, score);
    return 0;
}

int CRFSuiteXTagger::decode_nbest(const vector<vector<string> > &input, const vector<vector<double> > &weights)
{
    CRFSuite::ItemSequence xseq = fromVectorVector(input, weights);

    tagger.set(xseq);
    nBest = tagger.viterbi_n(nPaths, scores, normalizedScores);
    return 0;
}
