qloud env: https://qloud.yandex-team.ru/projects/jupyter/salt

ssh to dev installation: `ssh master.development.salt.jupyter.prestable.qloud-d.yandex.net`

ssh to prod installation: `master.production.salt.jupyter.prestable.qloud-d.yandex.net`

apply all states to all users: `salt '*' state.apply --state-output=changes`

apply all states to one user: `salt '*username*' state.apply --state-output=changes`

apply one state to all user: `salt '*' state.apply <state_name> --state-output=changes`,
state name, for example - `user_env.pip_env` or `user_env`

restart jupyterhub service: `salt '*' state.apply jupyterhub.restart --state-output=terse`;
This is only state which is not included into the highstate!
Applying this will shut down all notebooks for all users!!!

Jobs cache housekeeping:
salt '*' saltutil.clear_job_cache hours=6

when changed pillar: `salt '*' saltutil.refresh_pillar`
