#!/usr/bin/env python3

import re
from pathlib import Path


def compress_cpp(txt: str):
    txt = re.sub(r'  +', ' ', txt)
    txt = re.sub(r'\n\n+', '\n', txt)
    txt = re.sub(r' ?\n ?', '\n', txt)
    return txt.strip()


def main():
    base = Path(__file__).resolve().parent / 'include' / 'ispan'
    for path in base.rglob('*'):
        rel = path.relative_to(base).as_posix()
        txt = compress_cpp(path.read_text(encoding='utf-8', errors='replace'))
        print(f"`{rel}`:\n```\n{txt}\n```\n\n")


if __name__ == '__main__':
    main()
