import * as http from 'http';
import app from './server';
import log from './server/lib/log';

const { QLOUD_HTTP_PORT: port } = process.env;

const server = http.createServer(app);

process.on('SIGUSR2', () => {
    app.set('stopped', true);
});

server.listen(port, (err: Error) => {
    if (err) {
        log.error('Server startup failed');
        log.error(err);

        return;
    }

    log.info(`Server started on port ${port}`);
});
