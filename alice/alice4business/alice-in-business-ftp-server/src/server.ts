import http from 'http';
import { FileSystem, FtpConnection, FtpSrv } from 'ftp-srv';
import fs from 'fs';
import path from 'path';

import app from './app';
import log from './lib/log';
import utils from './utils';

const ROOT_DIR = path.resolve(__dirname, './uploads');
const HTTP_PORT = 80;
const FTP_PORT = 21;
const PASV_MIN_PORT = 65500;
const PASV_MAX_PORT = 65504;

const HttpServer = http.createServer(app);
HttpServer.timeout = 120000;

interface IFtpServerParams {
    on(event: "login", listener: (
        data: {
            connection: FtpConnection,
            username: string,
            password: string
        },
        resolve: (config: {
            fs?: FileSystem,
            root?: string,
            cwd?: string,
            blacklist?: Array<string>,
            whitelist?: Array<string>
        }) => void,
        reject: (err?: Error) => void
    ) => void): this;
    listen(): unknown;
}

const FtpServer: IFtpServerParams = new FtpSrv({
    url: `ftps://[::]:${FTP_PORT}`,
    pasv_url: '[::]',
    pasv_min: PASV_MIN_PORT,
    pasv_max: PASV_MAX_PORT,
    anonymous: false,
    greeting: ['Соединение установлено'],
    tls: {
        key: fs.readFileSync(`${__dirname}/certs/server-key.pem`),
        cert: fs.readFileSync(`${__dirname}/certs/server-cert.pem`),
        ca: [fs.readFileSync(`${__dirname}/certs/server-cert.pem`)],
        passphrase: 'qwerty',
    },
});

FtpServer.on('login', ({connection, username, password}, resolve, reject)  => {
    // TODO: Validate username
    const userDir = path.resolve(ROOT_DIR, username);
    const authResponse = utils.authorization(username, password)
    try {
        if (authResponse.status === 'ok') {
            if (authResponse.visitor === 'admin') {
                resolve({root: ROOT_DIR});
            } else {
                if(!fs.existsSync(userDir)) {
                    fs.mkdir(userDir, function(err) {
                        reject(err || undefined)
                    });
                }
                resolve({root: userDir, cwd: '/'});
            }
        } else {
            reject(Error('Bad username or password'));
        }
    } catch {
        reject(Error('Something went wrong'));
    }

});

FtpServer.listen();
HttpServer.listen(HTTP_PORT, () => {
    log.info(`Server started on port ${HTTP_PORT}`)
});

