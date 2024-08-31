#!/usr/bin/env node

import { createServer, request } from 'http';
import { writeFile, existsSync, mkdirSync } from 'fs';

import { join } from 'path';
import { program } from 'commander';
import { format } from 'date-fns';
import { logger } from './common/logger';

program.option('-p, --port <number>', 'listening port', '10000');
program.option('-d, --dist <string>', 'Ammo dist dir', 'ammos');
program.option('-f, --forward-to-port <number>', 'Port on which real backend stands', '10001');
program.option('-h, --forward-to-host <string>', 'Host on which real backend stands', 'localhost');

program.parse();
const options = program.opts();
const DIR = join(process.cwd(), options.dist);
if (!existsSync(DIR)) {
    mkdirSync(DIR);
}
const server = createServer((req, res) => {
    const fname = `${format(new Date(), 'yyyy-MM-dd_HH:mm:ss:SSS')}_${req.url?.slice(1)}.ammo`;
    const forward = request({
        host: options.forwardToHost,
        port: options.forwardToPort,
        path: req.url,
        headers: req.headers,
        method: req.method,
    }, fres => {
        fres.on('data', chunk => {
            res.write(chunk);
        });
        fres.on('end', () => {
            res.end();
        });
    });
    const chunks: Uint8Array[] = [];
    req.on('data', chunk => {
        chunks.push(chunk);
        forward.write(chunk);
    });
    req.on('end', () => {
        forward.end();
        const ammo: Array<Buffer | Uint8Array> = [Buffer.from(`${req.method} ${req.url} HTTP/${req.httpVersion}\n`)];
        const headers = Object.entries(req.headers);
        headers.forEach(([k, v], i) => {
            ammo.push(Buffer.from(`${k}: ${v}`));
            if (i < headers.length - 1) {
                ammo.push(Buffer.from('\n'));
            }
        });
        ammo.push(Buffer.from('\r\n\r\n'));
        ammo.push(...chunks);
        let ammoBuf = Buffer.concat(ammo);
        ammoBuf = Buffer.concat([Buffer.from(ammoBuf.byteLength + '\n'), ammoBuf]);
        writeFile(join(DIR, fname), ammoBuf, err => {
            if (err) {
                logger.error(err);
            } else {
                logger.info('Writing ' + fname);
            }
        });
    });
});

server.listen(options.port, () => {
    logger.info('Ammo proxy listening on ' + options.port);
});
