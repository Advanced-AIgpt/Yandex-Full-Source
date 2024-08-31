import tornado.httputil
import tornado.httpclient
import hashlib
import json


IGNORE_HEADERS = ["host", "transfer-encoding"]


def get_canonical_headers(headers: tornado.httputil.HTTPHeaders):
    res = []
    for name, value in headers.get_all():
        name = name.lower()
        if name in IGNORE_HEADERS:
            continue
        res.append([str(name), str(value.strip())])
    res.sort()
    return res


def get_canonical_request(req: tornado.httputil.HTTPServerRequest):
    if isinstance(req, tornado.httputil.HTTPServerRequest):
        return {
            "method": str(req.method.upper()),
            "uri": str(req.uri),
            "headers": get_canonical_headers(req.headers),
            "body": req.body,
        }
    if isinstance(req, tornado.httpclient.HTTPRequest):
        return {
            "method": str(req.method.upper()),
            "uri": str(req.url),
            "headers": get_canonical_headers(req.headers),
            "body": req.body,
        }


def get_canonical_request_hash(req):
    h = hashlib.sha256()
    h.update(json.dumps(req, sort_keys=True).encode("utf-8"))
    return h.hexdigest()


def get_canonical_response(resp: tornado.httpclient.HTTPResponse):
    return {
        "code": resp.code,
        "reason": str(resp.reason),
        "headers": get_canonical_headers(resp.headers),
        "body": resp.body,
    }


def make_request_from_canonical(canon_req, url=""):
    headers = tornado.httputil.HTTPHeaders()
    for name, value in canon_req["headers"]:
        headers.add(name, value)

    return tornado.httpclient.HTTPRequest(
        url=(url + canon_req["uri"]),
        method=canon_req["method"],
        headers=headers,
        body=canon_req["body"],
        allow_nonstandard_methods=True,
    )
