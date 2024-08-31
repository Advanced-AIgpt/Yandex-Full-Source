#include "py_nlg_renderer.h"
#include <library/cpp/pybind/v2.h>
#include <contrib/python/py3c/py3c.h>

MODULE_INIT_FUNC(bindings) {
    ::NAlice::NNlg::NLibrary::NPython::NNlgRenderer::NBindings::InitNlgRendererCreateFunctions();

    ::NPyBind::TPyModuleDefinition::InitModule("bindings");
    ::NAlice::NNlg::NLibrary::NPython::NNlgRenderer::NBindings::InitNlgRendererClasses();
    return ::NPyBind::TPyModuleDefinition::GetModule().M.RefGet();
}
