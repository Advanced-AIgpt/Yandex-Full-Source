import hashlib
import sys
from collections import defaultdict


def build_dialogs(comments, extra_columns={}):
    # expected comments format: list of dicts with ['user', 'id', 'pid', 'content']
    # pid = 0 means no parent comment

    # first tree comment uses it's comment id as dialog id, all children use this as their dialog ids
    comment_id_to_dialog_id = {}
    comment_id_to_content = {}
    comment_id_to_user_hash = {}

    tree = defaultdict(defaultdict)

    # Sometimes they are not sorted properly for some reason (probably DOM selector is not good enough)
    # TODO sort comments?
    # comments = sorted(comments, key=lambda comment: comment['.comment__body']['.comment__header']['time'])

    for comment in comments:
        content = comment['content']
        if content is None or len(content) == 0:
            continue

        user = comment['user']
        user_numeric_hash = int(hashlib.sha1(user.encode('utf-8')).hexdigest(), 16) % (10 ** 14)

        comment_id = comment['id']
        comment_pid = comment['pid']

        if comment_pid == '0':
            dialog_id = comment_id
        else:
            # Data corruption case, sometimes we do not see parent comments in data (but in reality they are present),
            # therefore, trying to make it a root
            if comment_pid not in comment_id_to_dialog_id:
                dialog_id = comment_id
                comment_pid = '0'

            # Normal case, making dialog id as of the parent
            else:
                dialog_id = comment_id_to_dialog_id[comment_pid]

        comment_id_to_dialog_id[comment_id] = dialog_id

        tree[comment_pid][comment_id] = defaultdict(defaultdict)

        comment_id_to_content[comment_id] = content
        comment_id_to_user_hash[comment_id] = user_numeric_hash

    dialogs = []

    def dfs(node_id, current_dialog):
        if len(tree[node_id]) == 0 and len(current_dialog) > 0:
            dialogs.append(list(current_dialog))
        else:
            for child_node_id in tree[node_id]:
                current_dialog.append(child_node_id)
                dfs(child_node_id, current_dialog)
                current_dialog.pop(-1)

    # '0' because pid of root comments is '0'
    dfs('0', [])

    for dialog in dialogs:
        # all comments in the dialog should be in one dialog id
        if not all(comment_id_to_dialog_id[comment] == comment_id_to_dialog_id[dialog[0]] for comment in dialog):
            sys.stderr.write(str(comment_id_to_dialog_id))
            sys.stderr.write(str(dialog))
            raise ValueError()

        dialog_string = '\t'.join(
            ' '.join([str(comment_id_to_user_hash[id]), comment_id_to_content[id]]) for id in dialog)

        yield {'key': comment_id_to_dialog_id[dialog[0]], 'subkey': '', 'value': dialog_string, **extra_columns}


def build_contexts_from_dialog(dialog, n_contexts, strict_size=False):
    contexts_with_authors = dialog.split('\t')
    authors, contexts = [], []

    for context_with_author in contexts_with_authors:
        split = context_with_author.split(' ')
        author, context = split[0], ' '.join(split[1:])
        authors.append(author)
        contexts.append(context)

    if len(contexts) <= n_contexts:
        if strict_size:
            return []
        else:
            return [contexts]

    all_contexts = []
    for i in range(len(contexts) - n_contexts):
        all_contexts.append(contexts[i:i + n_contexts])
    return all_contexts
