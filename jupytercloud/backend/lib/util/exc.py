class JupyterCloudException(Exception):
    pass


class DiskResourceException(JupyterCloudException):
    pass


class ClientError(JupyterCloudException):
    pass
