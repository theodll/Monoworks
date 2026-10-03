#!/usr/bin/env python3
import argparse
import os
import shutil
import sys


def main():
    parser = argparse.ArgumentParser(description="Copy directories from project-level 'common' into a target runtime directory.")
    parser.add_argument("--source-dir", required=True, help="Path to the 'common' source directory")
    parser.add_argument("--dest-dir", required=True, help="Destination runtime directory for the target")
    args = parser.parse_args()

    src = os.path.abspath(args.source_dir)
    dst = os.path.abspath(args.dest_dir)

    if not os.path.isdir(src):
        print(f"Source directory '{src}' does not exist.", file=sys.stderr)
        return 1

    os.makedirs(dst, exist_ok=True)

    ok = True
    for name in os.listdir(src):
        s = os.path.join(src, name)
        if os.path.isdir(s):
            dest_path = os.path.join(dst, name)
            try:
                # Python 3.8+ supports dirs_exist_ok
                shutil.copytree(s, dest_path, dirs_exist_ok=True)
                print(f"Copied '{s}' -> '{dest_path}'")
            except TypeError:
                # older Python: remove dest if exists then copy
                if os.path.exists(dest_path):
                    try:
                        if os.path.isdir(dest_path):
                            shutil.rmtree(dest_path)
                        else:
                            os.remove(dest_path)
                    except Exception as e:
                        print(f"Failed to remove existing destination '{dest_path}': {e}", file=sys.stderr)
                        ok = False
                        continue
                try:
                    shutil.copytree(s, dest_path)
                    print(f"Copied '{s}' -> '{dest_path}'")
                except Exception as e:
                    print(f"Failed to copy '{s}' -> '{dest_path}': {e}", file=sys.stderr)
                    ok = False
        else:
            # skip files at top-level of common
            continue

    return 0 if ok else 2


if __name__ == '__main__':
    sys.exit(main())
