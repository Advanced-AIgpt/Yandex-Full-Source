import http from 'http';
import app from './app';
import config from './lib/config';
import log from './lib/log';
import configureTerminus from './lib/terminus';

const { QLOUD_HTTP_PORT: port = 8000 } = process.env;
const server = http.createServer(app);
server.timeout = config.server.timeout;

if (config.terminus.enabled) {
    configureTerminus(server);
}

server.listen(port, () => {
    log.info(`Server started on port ${port}`);
});
