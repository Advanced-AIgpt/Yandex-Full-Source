import json
import os
import sqlite3
from pathlib import Path
from shutil import chown

from notebook.base.handlers import IPythonHandler
from notebook.utils import url_path_join
from tornado import web
from tornado.httpclient import AsyncHTTPClient, HTTPRequest, HTTPClientError

UUID_RE = r"(?P<uuid>.+)"
TOKEN_RE = r"(?P<token>[^\]+)"


class VaultDatabase:
    def __init__(self, username, path=None):
        if path is None:
            path = Path.home() / ".jupyter" / "vault.sqlite"
        self.username = username
        self.path = path
        self.check_exists()

    def check_exists(self):
        con = sqlite3.connect(self.path)
        with con:
            con.execute(
                """CREATE TABLE IF NOT EXISTS Tokens (
                secret_uuid TEXT PRIMARY KEY,
                token TEXT NOT NULL)"""
            )
        chown(self.path, self.username)

    def retrieve_token(self, secret_uuid):
        con = sqlite3.connect(self.path)
        with con:
            cursor = con.execute("SELECT token FROM Tokens WHERE secret_uuid=?", (secret_uuid,))

            result = cursor.fetchone()
            if result is None:
                return None
            else:
                return result[0]

    def save_token(self, secret_uuid, token):
        con = sqlite3.connect(self.path)
        with con:
            con.execute("INSERT INTO Tokens (secret_uuid, token) VALUES (?, ?)", (secret_uuid, token))

    def remove_token(self, *, secret_uuid=None, token=None):
        assert bool(secret_uuid is not None) ^ bool(token is not None)

        con = sqlite3.connect(self.path)
        with con:
            if secret_uuid is not None:
                con.execute("DELETE FROM Tokens WHERE secret_uuid = ?", (secret_uuid,))
            elif token is not None:
                con.execute("DELETE FROM Tokens WHERE token = ?", (token,))


class ServerExtensionMixin:
    def write_error(self, status_code, **kwargs):
        if 400 <= status_code < 500:
            self.clear()
            self.set_status(status_code)
            self.write(kwargs.get("exc_info")[1].log_message)
        else:
            super()._write_error(status_code, **kwargs)


class VaultPingHandler(IPythonHandler, ServerExtensionMixin):
    def get(self):
        self.write("OK")


class TokenHandler(IPythonHandler, ServerExtensionMixin):
    async def post(self, uuid):
        username = self.get_current_user()["name"]
        vault_db = VaultDatabase(username)

        token = vault_db.retrieve_token(uuid)
        if token is None:
            self.log.info("Getting token for secret %s from master", uuid)
            try:
                token = await self._get_token_from_master(uuid)
            except HTTPClientError as e:
                self.log.warn("Error in post: %s, %s, %s", e.code, e.response, e.response.body)
                raise web.HTTPError(e.code, e.response.body)
            else:
                vault_db.save_token(uuid, token)
        else:
            self.log.info("Got token for secrets %s from db", uuid)
        self.log.info("Token get success: %s", token)

    async def _get_token_from_master(self, secret_uuid):
        headers = self.request.headers
        req = HTTPRequest(
            url=os.environ.get("JUPYTERHUB_API_URL") + f"/vault/token",
            method="POST",
            body=json.dumps({"uuid": secret_uuid}),
            headers=dict(Cookie=headers["Cookie"]),
        )

        AsyncHTTPClient.configure(None, defaults={"ssl_options": None})
        client = AsyncHTTPClient()

        try:
            response = await client.fetch(req)
            response.rethrow()
            return json.loads(response.body)["token"]
        except HTTPClientError as e:
            self.log.error("Error getting token from master: %s", e, exc_info=True)
            raise


class SecretHandler(IPythonHandler, ServerExtensionMixin):
    async def get(self, uuid):
        username = self.settings["user"]
        vault_db = VaultDatabase(username)

        token = vault_db.retrieve_token(uuid)
        if token is None:
            vault_db.remove_token(secret_uuid=uuid)
            raise web.HTTPError(404, f"No token in db for uuid {uuid}")

        self.write(await self._get_secret_from_master(token))

    async def _get_secret_from_master(self, token):
        hub_api_token = self.settings["hub_auth"].api_token

        req = HTTPRequest(
            url=os.environ.get("JUPYTERHUB_API_URL") + f"/vault/secret",
            method="POST",
            body=json.dumps({"token": token}),
            headers={"Authorization": f"token {hub_api_token}"},
        )

        AsyncHTTPClient.configure(None, defaults={"ssl_options": None})
        client = AsyncHTTPClient()

        try:
            response = await client.fetch(req)
            response.rethrow()
            return response.body
        except HTTPClientError as e:
            self.log.error("Error getting secret from master: %s", e, exc_info=True)
            raise


def setup_handlers(web_app):
    host_pattern = ".*$"
    vault_check_pattern = url_path_join(web_app.settings["base_url"], "/vault")
    token_pattern = url_path_join(web_app.settings["base_url"], f"/vault/token/{UUID_RE}")
    secret_pattern = url_path_join(web_app.settings["base_url"], f"/vault/secret/{UUID_RE}")

    web_app.add_handlers(
        host_pattern,
        [
            (vault_check_pattern, VaultPingHandler),
            (token_pattern, TokenHandler),
            (secret_pattern, SecretHandler),
        ],
    )
