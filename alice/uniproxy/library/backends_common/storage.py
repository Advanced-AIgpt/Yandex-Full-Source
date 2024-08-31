import xml.etree.cElementTree as etree

from alice.uniproxy.library.utils.deepcopyx import deepcopy

from tornado import gen
from tornado.httpclient import AsyncHTTPClient

from rtlog import null_logger

from alice.uniproxy.library.settings import config
from alice.uniproxy.library.global_counter import GlobalCounter
from alice.uniproxy.library.auth.tvm2 import tvm_client
from alice.uniproxy.library.async_http_client.rtlog_http_client import fetch_http


class BadResponse(Exception):
    def __init__(self, message, status):
        super(BadResponse, self).__init__("Http error %s: %s" % (status, message))
        self.status = status


class MdsStorage(object):
    def __init__(self):
        self.url_download = config["mds"]["download"]
        self.url_upload = config["mds"]["upload"]
        self.headers = deepcopy(config["mds"].get("headers", {}))
        self.namespace = config["mds"]["namespace"]
        self.ttl = config["mds"]["ttl"]
        self.disabled = config["mds"].get("disabled", False)
        self.tvm = config["mds"].get("tvm", {})
        self.upload_timeout = config["mds"].get("upload_timeout", 20)

    def link(self, name):
        return u"%sget-%s/%s" % (self.url_download, self.namespace, name)

    @gen.coroutine
    def service_ticket_mds(self, rt_log):
        ticket = yield tvm_client().service_ticket_for(self.tvm.get("mds_service_id"),
                                                       rt_log=rt_log, rt_log_label='service_ticket_for_mds')
        return ticket

    @gen.coroutine
    def save(self, name, content, rt_log=null_logger(), rt_log_label=''):
        if self.disabled:
            raise gen.Return(u"%sget-%s/null" % (self.url_download, self.namespace))

        # temporary counters for VOICESERV-2885
        if self.tvm:
            tvm_ticket = yield self.service_ticket_mds(rt_log)
            self.headers["X-Ya-Service-Ticket"] = tvm_ticket
            GlobalCounter.MDS_SAVE_WITH_TVM_SUMM.increment()
        else:
            GlobalCounter.MDS_SAVE_WITHOUT_TVM_SUMM.increment()

        tries = 3
        while tries > 0:
            url = u"%supload-%s/%s?expire=%s" % (self.url_upload, self.namespace, name, self.ttl)
            data = content if type(content) is bytes else content.getvalue()
            resp = yield fetch_http(
                AsyncHTTPClient(),
                url,
                method="POST",
                headers=self.headers,
                body=data,
                request_timeout=self.upload_timeout,
                raise_error=False,
                rt_log=rt_log,
                rt_log_label='{0}/upload'.format(rt_log_label))

            GlobalCounter.increment_error_code("mds", resp.code)

            if resp.code == 200:
                GlobalCounter.MDS_WRITTEN_BYTES_SUMM.increment(len(data))
                tree = etree.XML(resp.body)
                key = tree.get("key")
                raise gen.Return(u"%sget-%s/%s" % (self.url_download, self.namespace, key))
            if resp.code == 403:
                tree = etree.XML(resp.body)
                key = tree.find("key").text
                res = yield self.delete(key, rt_log=rt_log, rt_log_label=rt_log_label)
                if res:
                    tries -= 1
                    continue
            elif resp.code == 400:
                raise gen.Return(None)
            raise BadResponse(resp.body, resp.code)

    @gen.coroutine
    def delete(self, name, rt_log=null_logger(), rt_log_label=''):
        if self.disabled:
            raise gen.Return(True)

        url = u"%sdelete-%s/%s" % (self.url_upload, self.namespace, name)
        res = yield fetch_http(
            AsyncHTTPClient(),
            url,
            headers=self.headers,
            rt_log=rt_log,
            rt_log_label=rt_log_label)
        raise gen.Return(res.code == 200)
