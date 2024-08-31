#pragma once

#include "http_request.h"

class TConfig;

namespace NBASS {
namespace NTestUsersDetails {
class TUserManager;
} // namespace NTestUsersDetails

/** HTTP JSON handler for /test_user path.
 * @code Add/Update user:
{
   "args" : {
      "token" : "SUPER_PUPPER_MEGA_TOKEN",
      "uuid" : "deadbeefdeadbeefdeadbeef10000001",
      "password" : "super.duper.mega.password",
      "tags" : [
         "oauth",
         "ya_music",
         "video"
      ],
      "login" : "bass.test.userXXXX",
      "client_ip" : null
   },
   "method" : "AddUser"
}
 * @endcode
 * @code GetUser
{
   "args" : {
      "tags" : [
         "oauth"
      ],
      "timeout" : 1,
      //"login" : "bass.test.userXXXX"
   },
   "method" : "GetUser"
}
 * @endcode
 */
class TTestUsersRequestHandler : public TJsonHttpRequestHandler {
public:
    HttpCodes DoJsonReply(TGlobalContextPtr globalCtx, const NSc::TValue& request,
                          const TParsedHttpFull& httpRequest, const THttpHeaders& httpHeaders,
                          NSc::TValue* response) override;

    static void RegisterHttpHandlers(THttpHandlersMap* handlers, TGlobalContextPtr globalCtx);

private:
    TTestUsersRequestHandler(TGlobalContextPtr globalCtx);

    HttpCodes GetUser(const NSc::TValue& user, NSc::TValue* out);
    HttpCodes ReleaseUser(const NSc::TValue& user, NSc::TValue* out);

    const TString& GetReqIdClass() const override;

private:
    TGlobalContextPtr GlobalCtx;
    THolder<NTestUsersDetails::TUserManager> UserManager;
};

} // namespace NBASS
