import argparse
import copy
import datetime
import os
import re
import sys
from pathlib import Path
from typing import cast

import polib

# for --@ hellow, world
TRANSLATION_COMMENT_PATTERN = re.compile(r"^\s*--@\s*(?P<text>\S(?:.*\S)?)\s*$")
# for Tr("key"
CALL_KEY_PATTERN = re.compile(r'Tr\s*\(\s*["\'](?P<key>[^"\']+)["\']')
# for {text}
PLACEHOLDER_PATTERN = re.compile(r"\{[a-zA-Z0-9_]+\}")


def existing_dir(value: str) -> Path:
    p = Path(value).expanduser()
    if not p.is_dir():
        raise argparse.ArgumentTypeError(f"Not a existing path: {value}")
    return p


def extract_from_file(file_path: str, rel_path: str) -> list[tuple[str, str, int]]:
    results: list[tuple[str, str, int]] = []

    with open(file_path, "r", encoding="utf-8") as fp:
        lines = fp.readlines()
    pending_text: str | None = None
    pending_line: int = 0
    lookahead_budget: int = 0

    for line_idx, raw_line in enumerate(lines, start=1):
        line = raw_line.strip()
        prefix_match = TRANSLATION_COMMENT_PATTERN.match(line)
        if prefix_match:
            pending_text = prefix_match.group("text").strip()
            pending_line = line_idx
            lookahead_budget = (
                5  # Search downward for the corresponding key, up to 5 lines.
            )
            continue
        if pending_text:
            key_match = CALL_KEY_PATTERN.search(raw_line)
            if key_match:
                key = key_match.group("key")
                results.append((key, pending_text, pending_line))
                pending_text = None
                lookahead_budget = 0
            else:
                if line and not line.startswith("--"):
                    lookahead_budget -= 1
                    if lookahead_budget <= 0:
                        pending_text = None
                continue
    return results


def build_po_file(
    entries: dict[str, polib.POEntry],
    output_path: str,
    lang: str | None = None,
    fill_msgstr: bool = False,
):
    """Build a .pot or .po file from extracted entries."""
    po = polib.POFile()
    now = datetime.datetime.now().strftime("%Y-%m-%d %H:%M%z")
    metadata = {
        "Project-Id-Version": "Serenkai",
        "Report-Msgid-Bugs-To": "",
        "POT-Creation-Date": now,
        "PO-Revision-Date": now if lang else "YEAR-MO-DA HO:MI+ZONE",
        "Last-Translator": "FULL NAME <EMAIL@ADDRESS>",
        "Language-Team": "LANGUAGE <LL@li.org>",
        "MIME-Version": "1.0",
        "Content-Type": "text/plain; charset=UTF-8",
        "Content-Transfer-Encoding": "8bit",
    }
    if lang:
        metadata["Language"] = lang
    po.metadata = metadata

    # Output sorted alphabetically by Key.
    for _, entry in sorted(entries.items()):
        if fill_msgstr:
            # Prevent the original msgstr from being modified.
            e = copy.copy(entry)
            e.msgstr = e.msgid
            po.append(e)
        else:
            po.append(entry)

    os.makedirs(os.path.dirname(os.path.abspath(output_path)), exist_ok=True)
    po.save(output_path)


def build_pot(entries: dict[str, polib.POEntry], output_path: str):
    build_po_file(entries, output_path, lang=None, fill_msgstr=False)


def build_en_po(entries: dict[str, polib.POEntry], output_path: str):
    build_po_file(entries, output_path, lang="en_US", fill_msgstr=True)


def main():
    parser = argparse.ArgumentParser(
        description="Extract localization strings from Luau scripts into a .pot file."
    )

    parser.add_argument(
        "--src", required=True, type=existing_dir, help="Source directory to scan"
    )

    parser.add_argument(
        "--out",
        required=True,
        help="Output .pot file path",
    )

    parser.add_argument(
        "--en-out",
        required=False,
        default=None,
        help="Optional output .po file path for en_US locale",
    )

    args = parser.parse_args()

    src = cast(Path, args.src)
    out = cast(Path, args.out)

    src_dir = os.path.abspath(src)
    out_file = os.path.abspath(out)

    entries: dict[str, polib.POEntry] = {}
    conflict_count = 0

    for root, _, files in os.walk(src_dir):
        for f in files:
            if not f.endswith((".luau", ".lua")):
                continue

            full_path = os.path.join(root, f)
            # Extract the path relative to the project root, ensuring the script is run from the project root directory.
            rel_path = os.path.relpath(full_path, os.getcwd())

            file_results = extract_from_file(full_path, rel_path)

            for key, text, line_no in file_results:
                if key in entries:
                    entry = entries[key]

                    if entry.msgid != text:
                        conflict_count += 1
                        first_ref = (
                            entry.occurrences[0]
                            if entry.occurrences
                            else ("unknown", 0)
                        )
                        print(
                            f"[Warning] Key conflict for '{key}':\n"
                            f"  - '{entry.msgid}' at {first_ref[0]}:{first_ref[1]}\n"
                            f"  - '{text}' at {rel_path}:{line_no}",
                            file=sys.stderr,
                        )
                    entry.occurrences.append((rel_path, str(line_no)))
                else:
                    has_format = bool(PLACEHOLDER_PATTERN.search(text))
                    flags = ["python-brace-format"] if has_format else []

                    entry = polib.POEntry(
                        msgctxt=key,
                        msgid=text,
                        msgstr="",
                        occurrences=[(rel_path, str(line_no))],
                        flags=flags,
                    )

                    entries[key] = entry

    build_pot(entries, out_file)
    print(f"Done! Successfully extracted {len(entries)} entries to {out_file}")

    if args.en_out:
        en_out_file = os.path.abspath(args.en_out)
        build_en_po(entries, en_out_file)
        print(f"Done! Successfully generated en_US PO file to {en_out_file}")

    if conflict_count > 0:
        print(f"[Warning] Found {conflict_count} key conflict(s).", file=sys.stderr)


if __name__ == "__main__":
    main()
