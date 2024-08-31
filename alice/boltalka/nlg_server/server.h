#pragma once
#include "nlg_model.h"

#include <quality/deprecated/Misc/commonStdAfx.h>
#include <quality/deprecated/net_http/http_request.h>
#include <quality/deprecated/net_http/http_server.h>

#include <util/folder/path.h>
#include <library/cpp/cgiparam/cgiparam.h>

namespace NNlgServer {

class TNlgServer {
public:
    explicit TNlgServer(int port, int threadCount);
    void LoadModels(const TFsPath &modelDir, TStringBuf mode);
    void HandleRequests();

private:
    void Respond(SOCKET socket, TCgiParameters params, ui64 workerId);

private:
    int Port;
    int ThreadCount;
    THashMap<TString, INlgModelSPtr> NlgModels;
};

}

