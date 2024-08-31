#include "ru_yandex_alice_nlu_libs_fstnormalizer_FstNormalizer.h"

#include "jni.h"
#include <library/cpp/langs/langs.h>
#include <alice/nlu/libs/request_normalizer/request_normalizer.h>

#include <util/generic/strbuf.h>

/*
 * Class:     ru_yandex_alice_nlu_libs_fstnormalizer_FstNormalizer
 * Method:    normalize
 * Signature: (Ljava/lang/String;Ljava/lang/String;)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_ru_yandex_alice_nlu_libs_fstnormalizer_FstNormalizer_normalize
  (JNIEnv *jenv, jobject, jstring lang, jstring text) {
  	const char* k_lang = jenv->GetStringUTFChars(lang, 0);
  	const char* k_text = jenv->GetStringUTFChars(text, 0);
  	auto l = LanguageByName(TStringBuf(k_lang, jenv->GetStringUTFLength(lang)));
  	auto res =  NNlu::TRequestNormalizer::Normalize(l, TStringBuf(k_text, jenv->GetStringUTFLength(text)));
  	jstring normalized = jenv->NewStringUTF(res.c_str());
  	jenv->ReleaseStringUTFChars(lang, k_lang);
  	jenv->ReleaseStringUTFChars(text, k_text);
  	return normalized;
  }