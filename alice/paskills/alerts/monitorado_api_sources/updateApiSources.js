#!/usr/bin/env node

const execa = require("execa");
const R = require("ramda");

if (!process.argv[2]) {
    console.error("Нужно передать host api в скрипт");
    process.exit(1);
}

const unistat = JSON.parse(
    execa.commandSync(`curl ${process.argv[2]}/unistat`)
        .stdout
);

const httpSources = R.pipe(
    R.filter(([signal]) => signal.startsWith("http_source")),
    R.map(([signal]) => /http_source_([0-9a-zA-Z-]+)/.exec(signal)[1]),
    R.uniq
)(unistat);

console.log(httpSources.map((source) =>
`
http_source_${source}_errors:
  signal: or(perc(unistat-http_source_${source}_requests_error_summ, unistat-http_source_${source}_requests_total_summ), 0)
  warn: 5
  crit: 10
`
).join(''));
