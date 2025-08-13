#!/usr/bin/env python3

import re
from pathlib import Path


def compress_cpp(txt: str):
    txt = re.sub(r'  +', ' ', txt)
    txt = re.sub(r'\n\n+', '\n', txt)
    txt = re.sub(r' ?\n ?', '\n', txt)
    return txt.strip()


def main():
    base = Path(__file__).resolve().parent
    include_dir = base / 'include'
    readme = base / 'README.md'
    out_path = base / 'source.md'

    with out_path.open('w', encoding='utf-8', newline='\n') as out:

        rel = readme.relative_to(base).as_posix()
        txt = readme.read_text(encoding='utf-8', errors='replace')
        out.write(f"`{rel}`:\n```\n{txt}\n```\n\n")

        if include_dir.exists() and include_dir.is_dir():
            for path in include_dir.rglob('*'):
                if path.is_file():
                    rel = path.relative_to(base).as_posix()
                    txt = compress_cpp(path.read_text(encoding='utf-8', errors='replace'))
                    out.write(f"`{rel}`:\n```\n{txt}\n```\n\n")


if __name__ == '__main__':
    main()
