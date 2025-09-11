#!/usr/bin/env python3
import argparse
import gzip
import os
import shutil
import tarfile
import tempfile
import urllib.request
import zipfile
from pathlib import Path

from file_lock import FileLock


def snap(extract_dir: Path, src: dict):
    ...


def konnect(extract_dir: Path, src: dict):
    ...


tags = ['all', 'tiny', 'small', 'medium', 'big', 'huge', 'gigantic']
sources = [
    {
        'code': 'wiki-Talk',
        'url': 'https://snap.stanford.edu/data/wiki-Talk.txt.gz',
        'archive': '.txt.gz',
        'norm': snap,
        'tags': ['all', 'small']
    },
    {
        'code': 'soc-LiveJournal1',
        'url': 'https://snap.stanford.edu/data/soc-LiveJournal1.txt.gz',
        'archive': '.txt.gz',
        'norm': snap,
        'tags': ['all', 'medium']
    },
    {
        'code': 'zhishi-baidu-internallink',
        'url': 'http://konect.cc/files/download.tsv.zhishi-baidu-internallink.tar.bz2',
        'archive': '.tar.bz2',
        'norm': konnect,
        'tags': ['all', 'medium']
    }
]
codes = [source['code'] for source in sources]


def download(archive: Path, url: str):
    archive.parent.mkdir(parents=True, exist_ok=True)
    with urllib.request.urlopen(url) as response:
        with open(archive, 'wb') as f:
            while True:
                chunk = response.read(16 * 1024)
                if not chunk:
                    break
                f.write(chunk)


def extract(extract_dir: Path, archive: Path):
    if zipfile.is_zipfile(archive):
        extract_dir.mkdir(parents=True, exist_ok=True)
        with zipfile.ZipFile(archive) as z:
            z.extractall(extract_dir)
    elif tarfile.is_tarfile(archive):
        extract_dir.mkdir(parents=True, exist_ok=True)
        with tarfile.open(archive) as t:
            t.extractall(extract_dir)
    elif archive.suffix in ['.gz', '.gzip']:
        extract_dir.mkdir(parents=True, exist_ok=True)
        with gzip.open(archive, 'rb') as g:
            with open(extract_dir / archive.stem, 'wb') as f:
                # noinspection PyTypeChecker
                shutil.copyfileobj(g, f)
    else:
        raise ValueError(f"{archive} is of unsupported archive type")


def process_source(source, output_dir, temp_dir):
    output = output_dir / (source['code'] + '.txt')

    if not os.path.exists(output):
        with tempfile.TemporaryDirectory(dir=temp_dir) as temporary_dir:
            temporary_dir = Path(temporary_dir)

            archive = temporary_dir / (source['code'] + source['archive'])
            extract_dir = temporary_dir / source['code']

            download(archive, source['url'])
            extract(extract_dir, archive)

            source['norm'](extract_dir, source)

            result = temporary_dir / source['code'] / (source['code'] + '.txt')
            result.rename(output)


def main():
    parser = argparse.ArgumentParser(description='fetch and convert graphs from given code and tag filters')
    parser.add_argument('-d', '--directory', nargs='?', type=str)
    parser.add_argument('items', nargs='*', type=str)
    args = parser.parse_args()

    output_dir = (Path(args.directory) or (Path(os.getcwd()) / 'graph')).absolute()
    output_dir.mkdir(parents=True, exist_ok=True)

    temp_dir = output_dir / '.temp'
    temp_dir.mkdir(parents=True, exist_ok=True)

    with FileLock(temp_dir / '.lock'):
        for source in sources:
            if source['code'] in args.items or any([tag in args.items for tag in source['tags']]):
                try:
                    process_source(source, output_dir, temp_dir)
                    print(f"info: fetched {source['code']}")
                except BaseException as e:
                    print(f"warning: failed to fetch {source['code']} - {e}")


if __name__ == '__main__':
    main()
