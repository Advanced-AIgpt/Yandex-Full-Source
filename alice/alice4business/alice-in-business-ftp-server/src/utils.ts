interface AuthorizationOkResponse {
    status: 'ok',
    visitor: 'admin' | 'hotel'
}
interface AuthorizationNotOkResponse {
    status: 'not ok'
}

type AuthorizationResponse = AuthorizationOkResponse | AuthorizationNotOkResponse;

const authorization = (login: string, password: string): AuthorizationResponse => {
    if(login === 'alice4business_is_cool' && password === 'Xcx9mZTsIHis') {
        return {status: 'ok', visitor: 'admin'}
    } else if(login === 'hotel' && password === 'V168rqubUMOw'){
        return {status: 'ok', visitor: 'hotel'}
    } else {
        return {status: 'not ok'}
    }
}

export default {
    authorization
}
