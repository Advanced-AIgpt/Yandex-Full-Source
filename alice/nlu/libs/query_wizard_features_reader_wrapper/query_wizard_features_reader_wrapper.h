#pragma once

#include <stddef.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/charset/utf8.h>

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _WIN32
#ifdef _WINDLL
#define EXPORT __declspec(dllexport)
#else
#define EXPORT __declspec(dllimport)
#endif
#else
#define EXPORT
#endif

#define FEATURES_NUMBER 19

typedef void ReaderHandle;

struct TFragmentFeatures {
    size_t Start;
    size_t Length;
    TString Fragment;
    TString NormalizedFragment;
    TVector<double> Features;
};

/**
 * Create query wizard features reader handle
 * @return
 */
EXPORT ReaderHandle* ReaderCreate();

/**
 * Delete reader handle
 * @param calcer
 */
EXPORT void ReaderDelete(ReaderHandle* reader);

/**
 * If error occured will return stored exception message.
 * If no error occured, will return invalid pointer
 * @return
 */
EXPORT const char* GetErrorString();

/**
 * Load trie and data from file
 * @param calcer
 * @param filename
 * @return false if error occured
 */
EXPORT bool LoadTrie(
    ReaderHandle* reader,
    const char* triePath,
    const char* dataPath);

/**
 * Get SERP features for query
 * @param reader handle
 * @param text fragment
 * @param result pointer to user allocated results vector
 * @param resultSize result size
 * @return false if error occured
 */
EXPORT bool GetFeaturesForTextFragments(
    ReaderHandle* reader,
    const char* text,
    TVector<TFragmentFeatures>* result);

#if defined(__cplusplus)
}
#endif
