# -*- coding: utf-8 -*-
from .common import with_qloud_format, HOST_FQDN, TIME_FMT
from asyncio.subprocess import Process
import json
from datetime import datetime


@with_qloud_format()
async def test_prefix_and_exit_code(process: Process):
    process.stdin.write(b"1;2;3;log record\n")
    out = await process.stdout.readline()
    assert out.startswith(b"1;2;3;")
    assert b"log record" in out

    process.stdin.write(b"4;5;6;another; log; record; \n")
    out = await process.stdout.readline()
    assert out.startswith(b"4;5;6;")
    assert b"another; log; record; " in out

    process.terminate()
    assert 0 == await process.wait()


@with_qloud_format()
async def test_escaping_json_in_messages(process: Process):
    # create doom's day message
    record = {"key": 1, "\\key\\": [True, False, None], "\"key\"": ["первое\nвторое\r\nкомпот", "\"c\"\t\"d\""]}
    for i in range(0, 10):
        record["internal\nmessage"] = "here it is: " + json.dumps(record)
    record = "SESSIONLOG: " + json.dumps(record)

    process.stdin.write(b"1;2;3;" + record.encode("utf-8") + b"\n")
    out = await process.stdout.readline()
    a, b, c, out_json = out.decode("utf-8").split(";", 3)
    assert a == "1"
    assert b == "2"
    assert c == "3"
    assert json.loads(out_json)["message"] == record


@with_qloud_format()
async def test_bad_input(process: Process):
    process.stdin.write(
        b"\n"
        b" \t  \t  \n"
        b"row without delimiters\n"
        b"too;few;delimiters\n"
        b"secret;rowId;offset;message;with;some;delimiters\n"  # the only valid line
    )
    out = await process.stdout.readline()

    secret, row_id, offset, log_data = out.decode("utf-8").split(";", 3)
    assert secret == "secret"
    assert row_id == "rowId"
    assert offset == "offset"
    assert "message;with;some;delimiters" in log_data


@with_qloud_format()
async def test_output_without_validation(process: Process):
    log_timestamp = datetime.now().timestamp()
    process.stdin.write(b"1;2;3;log; record; \n")
    out = await process.stdout.readline()

    secret, row_id, offset, log_data = out.decode("utf-8").split(";", 3)
    log_data = json.loads(log_data)

    assert secret == "1"
    assert row_id == "2"
    assert offset == "3"
    assert log_data["pushclient_row_id"] == "2"
    assert log_data["level"] == 20000
    assert log_data["levelStr"] == "INFO"
    assert log_data["loggerName"] == "stdout"
    assert log_data["@version"] == 1
    assert log_data["threadName"] == "qloud-init"
    assert log_data["qloud_project"] == "alice"
    assert log_data["qloud_application"] == "uniproxy"
    assert log_data["qloud_environment"] == "stable"
    assert log_data["qloud_component"] == "uniproxy"
    assert log_data["qloud_instance"] == "-"
    assert log_data["message"] == "log; record; "
    assert log_data["host"] == HOST_FQDN
    assert abs(datetime.strptime(log_data["@timestamp"], TIME_FMT).timestamp() - log_timestamp) < 10


@with_qloud_format("--validate-sessionlog")
async def test_output_with_sessionlog_validation(process: Process):
    # invalid JSONs but without SESSIONLOG prefix
    process.stdin.write(
        b'1;2;3;{"key": "value}\n'
        b'4;5;6;SESSIONLOK: {"key": "value}\n'
    )
    out = await process.stdout.readline()
    print(out)
    assert out.startswith(b'1;2;3;') and (b'{\\"key\\": \\"value}' in out)
    out = await process.stdout.readline()
    assert out.startswith(b'4;5;6;') and (b'SESSIONLOK: {\\"key\\": \\"value}' in out)

    # invalid JSONs with SESSIONLOG prefix
    process.stdin.write(
        b'1;2;3;SESSIONLOG: {"key": "value}\n'
        b'1;2;3;SESSIONLOG: {"key": {"key2": [1, 2, three]}}\n'
        b'1;2;3;SESSIONLOG: {"key": {"keSESSIONLOG: {"key": "value"}\n'
        b'X;X;X;\n'  # the only valid line
    )
    out = await process.stdout.readline()
    assert out.startswith(b'X;X;X;')

    # valid JSON with SESSIONLOG prefix
    process.stdin.write(
        b'1;2;3;SESSIONLOG: {"key": "value", "key2": [1, 2, 3]}\n'
    )
    out = await process.stdout.readline()
    assert out.startswith(b'1;2;3;') and (b'SESSIONLOG: {\\"key\\": \\"value\\", \\"key2\\": [1, 2, 3]}' in out)
