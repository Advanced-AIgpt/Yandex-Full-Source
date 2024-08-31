import config from "../../lib/config"
import RestApiError from "../../lib/errors/restApi";
import { oauthGot } from "./utils"

export const getTokenByCode = async (code: string): Promise<string> => {
    return oauthGot("/token", {
        method: "post",
        body: {
            grant_type: "authorization_code",
            code,
            client_id: config.quasar.oauthClient.id,
            client_secret: config.quasar.oauthClient.secret
        },
        form: true
    }).then(resp => {
        if (!resp.access_token) {
            throw new RestApiError('Failed to exchange kolonkish code to token', 500, {
                payload: { oauthResponse: resp },
            });
        }
        return resp.access_token;
    });
}