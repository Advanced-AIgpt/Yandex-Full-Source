#pragma once

#ifndef CRFSUITEXTAGGER_H
#define CRFSUITEXTAGGER_H

#include <string>
#include <vector>
#include <crfsuite_api.hpp>
#include <stdio.h>
#include <stdlib.h>

using namespace std;

class CRFSuiteXTagger
{
    string model;
    int nPaths;
    vector<vector<string> > nBest;
    vector<double> scores;
    CRFSuite::Tagger tagger;
    bool normalizedScores;

    CRFSuite::ItemSequence fromVectorVector(const vector<vector<string> >& input, const vector<vector<double> >& weights);

    bool loadModelFromFile(const char* filename);

public:

    CRFSuiteXTagger();

    int loadFromFile(const string& FileName);

    //getters
    const vector<vector<string> > getNBest();
    const vector<double> getScores();
    const string getModel();
    int getNPaths();
    bool getNormalizedScores();

    //setters
    void setModel(const string& Model);
    void setNPaths(int NPaths);
    void setNormalizedScores(bool NormalizedScores);

    int decode(const vector<vector<string> >& input, const vector<vector<double> >& weights);

    int decode_nbest(const vector<vector<string> >& input, const vector<vector<double> >& weights);
};
#endif // CRFSUITEXTAGGER_H
