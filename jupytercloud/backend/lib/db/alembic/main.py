from alembic import util
from alembic.arcadia.script import ArcadiaScriptDirectory
from alembic.config import main as _main
from alembic.script import Script, revision, write_hooks
from alembic.util import compat

from jupytercloud.backend.lib.util.paths import get_arcadia_root


class JCScriptDirectory(ArcadiaScriptDirectory):
    def generate_revision(
        self,
        revid,
        message,
        head=None,
        refresh=False,
        splice=False,
        branch_labels=None,
        version_path=None,
        depends_on=None,
        **kw,
    ):
        if head is None:
            head = 'head'

        try:
            Script.verify_rev_id(revid)
        except revision.RevisionError as err:
            compat.raise_from_cause(util.CommandError(err.args[0]))

        with self._catch_revision_errors(
            multiple_heads=(
                'Multiple heads are present; please specify the head '
                'revision on which the new revision should be based, '
                'or perform a merge.'
            ),
        ):
            heads = self.revision_map.get_revisions(head)

        if len(set(heads)) != len(heads):
            raise util.CommandError('Duplicate head revisions specified')

        create_date = self._generate_create_date()

        version_path = get_arcadia_root() / 'jupytercloud/backend/db/alembic/versions'
        version_path = str(version_path)

        if self.version_locations:
            self._ensure_directory(version_path)

        path = self._rev_path(version_path, revid, message, create_date)

        if not splice:
            for head in heads:
                if head is not None and not head.is_head:
                    raise util.CommandError(
                        'Revision %s is not a head revision; please specify '
                        '--splice to create a new branch from this revision'
                        % head.revision,
                    )

        if depends_on:
            with self._catch_revision_errors():
                depends_on = [
                    dep
                    if dep in rev.branch_labels  # maintain branch labels
                    else rev.revision  # resolve partial revision identifiers
                    for rev, dep in [
                        (self.revision_map.get_revision(dep), dep)
                        for dep in util.to_list(depends_on)
                    ]
                ]

        self._generate_template(
            str(get_arcadia_root() / 'jupytercloud/backend/db/alembic/script.py.mako'),
            path,
            up_revision=str(revid),
            down_revision=revision.tuple_rev_as_scalar(
                tuple(h.revision if h is not None else None for h in heads),
            ),
            branch_labels=util.to_tuple(branch_labels),
            depends_on=revision.tuple_rev_as_scalar(depends_on),
            create_date=create_date,
            comma=util.format_as_comma,
            message=message if message is not None else ('empty message'),
            **kw,
        )

        post_write_hooks = self.hook_config
        if post_write_hooks:
            write_hooks._run_hooks(path, post_write_hooks)

        try:
            script = Script._from_path(self, path)
        except revision.RevisionError as err:
            compat.raise_from_cause(util.CommandError(err.args[0]))
        if branch_labels and not script.branch_labels:
            raise util.CommandError(
                'Version %s specified branch_labels %s, however the '
                'migration file %s does not have them; have you upgraded '
                'your script.py.mako to include the '
                "'branch_labels' section?"
                % (script.revision, branch_labels, script.path),
            )

        self.revision_map.add_revision(script)
        return script


def main(argv=None, prog=None, **kwargs):
    import alembic.command

    alembic.command.ArcadiaScriptDirectory = JCScriptDirectory

    _main(argv=argv, prog=prog, **kwargs)
