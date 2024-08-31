import vh
from .pipelines import movies_pipeline


def main():
    with vh.Graph() as g:
        movies_pipeline()

    info = vh.run(g, quota='dialogs', yt_token_secret='artemkorenev_yt_token', yt_proxy='hahn').get_workflow_info()
    print('https://nirvana.yandex-team.ru/flow/{}'.format(info.workflow_id))


if __name__ == '__main__':
    main()
