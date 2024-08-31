import argparse
import yalibrary.svn as svn


def download_formulas(svn_url, dst_dir, revision=None):
    svn.svn_co(svn_url, dst_dir, revision=revision)


def parse_args():
    parser = argparse.ArgumentParser(
        description='Download Alice Megamind formulas',
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
        add_help=True)
    parser.add_argument('--dst-dir', default='megamind_formulas', help='Download path')
    parser.add_argument('--svn-url', default='svn+ssh://arcadia.yandex.ru/robots/trunk/alice/megamind', help='Download path')
    parser.add_argument('--svn-revision', help='SVN revision')
    return parser.parse_args()


def main(args):
    download_formulas(svn_url=args.svn_url, dst_dir=args.dst_dir)


if __name__ == '__main__':
    main(parse_args())
