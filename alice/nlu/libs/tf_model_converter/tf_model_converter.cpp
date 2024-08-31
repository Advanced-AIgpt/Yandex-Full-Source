#include "tf_model_converter.h"

#include <contrib/libs/tf/tensorflow/contrib/util/convert_graphdef_memmapped_format_lib.h>

#include <util/generic/yexception.h>
#include <util/system/fs.h>

void ConvertModelToMemmapped(
    const TString& pbModelFileName,
    size_t minConversionSizeBytes
) {
    const TString pbExtension = ".pb";
    const TString memmappedExtension = ".mmap";

    Y_ENSURE(
        pbModelFileName.EndsWith(pbExtension),
        "Model name must end with '" << pbExtension << "'"
    );
    Y_ENSURE(NFs::Exists(pbModelFileName));

    const TString memmappedModelFileName = TString::Join(
        pbModelFileName.substr(0, pbModelFileName.size() - pbExtension.size()),
        memmappedExtension
    );

    auto conversionStatus = tensorflow::ConvertConstantsToImmutable(
        pbModelFileName,
        memmappedModelFileName,
        minConversionSizeBytes
    );
    Y_ENSURE(
        conversionStatus.ok(),
        "Unable to convert model to memmapped format: " + conversionStatus.ToString()
    );
}
