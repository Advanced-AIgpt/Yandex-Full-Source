import base64
import numpy as np


# ==== Embedding utils ====

def sentence_embedding_to_base64(v):
    return str(base64.b64encode(v.astype(np.float16).tobytes()), 'utf-8')


def sentence_embedding_from_base64(s):
    return np.frombuffer(base64.b64decode(s), dtype=np.float16)


_EMBEDDING_NAME_TO_COLLECTION_KEY = {
    'embedding_dssm': ('Dssm', 'v1'),
}


def find_embedding_in_collection(embedding_collection, name):
    if name not in _EMBEDDING_NAME_TO_COLLECTION_KEY:
        return None
    type, version = _EMBEDDING_NAME_TO_COLLECTION_KEY[name]
    for embedding in embedding_collection:
        if embedding['Type'] == type and embedding['Version'] == version:
            return embedding['Data']
    return None


def find_embedding_in_dataset_row(dataset, row_index, embedding_name):
    if embedding_name in dataset.columns:
        return dataset.rows[row_index][dataset.columns[embedding_name]]
    if dataset.has_embeddings():
        return find_embedding_in_collection(dataset.get_embeddings(row_index), embedding_name)
    return None
