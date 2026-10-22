import argparse
from pathlib import Path

import polib
import pytest

from tools.extract_pot import (
    PLACEHOLDER_PATTERN,
    TRANSLATION_COMMENT_PATTERN,
    build_en_po,
    build_pot,
    existing_dir,
    extract_from_file,
    main,
)


def test_placeholder_pattern():
    assert bool(PLACEHOLDER_PATTERN.search("Hello {name}!")) is True
    assert bool(PLACEHOLDER_PATTERN.search("Items: {count_1}")) is True
    assert bool(PLACEHOLDER_PATTERN.search("Plain text without placeholder")) is False
    assert bool(PLACEHOLDER_PATTERN.search("Empty braces {}")) is False


def test_translation_comment_pattern():
    m1 = TRANSLATION_COMMENT_PATTERN.match("--@ Hello, world!")
    assert m1 is not None
    assert m1.group("text") == "Hello, world!"

    m2 = TRANSLATION_COMMENT_PATTERN.match("   --@   Leading and trailing spaces   ")
    assert m2 is not None
    assert m2.group("text") == "Leading and trailing spaces"

    assert TRANSLATION_COMMENT_PATTERN.match("-- regular comment") is None
    assert TRANSLATION_COMMENT_PATTERN.match("--@") is None


def test_existing_dir(tmp_path: Path):
    d = tmp_path / "valid_dir"
    d.mkdir()
    assert existing_dir(str(d)) == d

    f = tmp_path / "file.txt"
    f.write_text("content")
    with pytest.raises(argparse.ArgumentTypeError):
        existing_dir(str(f))

    with pytest.raises(argparse.ArgumentTypeError):
        existing_dir(str(tmp_path / "non_existent"))


def test_extract_basic(tmp_path: Path):
    script = tmp_path / "test.luau"
    script.write_text(
        "--@ Hello, world!\n"
        'local msg = Tr("ui.greeting")\n'
        "\n"
        "--@ Single quote text\n"
        "    local btn = Tr('ui.button')\n"
        "\n"
        "--@ Method argument\n"
        'ui:SetTitle(Tr("ui.title"))\n'
    )
    results = extract_from_file(str(script), "test.luau")
    assert results == [
        ("ui.greeting", "Hello, world!", 1),
        ("ui.button", "Single quote text", 4),
        ("ui.title", "Method argument", 7),
    ]


def test_extract_lookahead_budget(tmp_path: Path):
    # Within 5 non-comment lines with comments/blank lines in between
    script = tmp_path / "budget_ok.luau"
    script.write_text(
        "--@ Success text\n"
        'print("line 1")\n'
        "-- a lua comment\n"
        "\n"
        'print("line 2")\n'
        'local res = Tr("ui.ok")\n'
    )
    assert extract_from_file(str(script), "budget_ok.luau") == [
        ("ui.ok", "Success text", 1)
    ]

    # Exceeding 5 non-comment lines drops pending comment
    script_exceeded = tmp_path / "budget_exceeded.luau"
    script_exceeded.write_text(
        "--@ Failed text\n"
        "local a = 1\n"
        "local b = 2\n"
        "local c = 3\n"
        "local d = 4\n"
        "local e = 5\n"
        "local f = 6\n"
        'local res = Tr("ui.failed")\n'
    )
    assert extract_from_file(str(script_exceeded), "budget_exceeded.luau") == []


def test_extract_override_pending_comment(tmp_path: Path):
    # Successive comments before Tr should use the latest one
    script = tmp_path / "override.luau"
    script.write_text('--@ First text\n--@ Second text\nlocal res = Tr("ui.key")\n')
    assert extract_from_file(str(script), "override.luau") == [
        ("ui.key", "Second text", 2)
    ]


def test_build_pot(tmp_path: Path):
    out_file = tmp_path / "nested" / "output.pot"
    entries = {
        "z.key": polib.POEntry(
            msgctxt="z.key",
            msgid="Z text",
            msgstr="",
            occurrences=[("z.luau", "10")],
        ),
        "a.key": polib.POEntry(
            msgctxt="a.key",
            msgid="A text {name}",
            msgstr="",
            occurrences=[("a.luau", "5")],
            flags=["python-brace-format"],
        ),
    }

    build_pot(entries, str(out_file))
    assert out_file.exists()

    po = polib.pofile(str(out_file))
    assert len(po) == 2
    assert po.metadata["Project-Id-Version"] == "Serenkai"
    assert po.metadata["Content-Type"] == "text/plain; charset=UTF-8"

    # Entries sorted alphabetically by key
    assert po[0].msgctxt == "a.key"
    assert po[0].msgid == "A text {name}"
    assert po[0].flags == ["python-brace-format"]
    assert po[0].occurrences == [("a.luau", "5")]

    assert po[1].msgctxt == "z.key"
    assert po[1].msgid == "Z text"
    assert po[1].occurrences == [("z.luau", "10")]


def test_build_en_po(tmp_path: Path):
    out_file = tmp_path / "nested" / "en_US.po"
    entries = {
        "ui.start": polib.POEntry(
            msgctxt="ui.start",
            msgid="Start Game",
            msgstr="",
            occurrences=[("view.luau", "1")],
        ),
    }

    build_en_po(entries, str(out_file))
    assert out_file.exists()

    po = polib.pofile(str(out_file))
    assert len(po) == 1
    assert po.metadata["Project-Id-Version"] == "Serenkai"
    assert po.metadata["Language"] == "en_US"
    assert po.metadata["PO-Revision-Date"] != "YEAR-MO-DA HO:MI+ZONE"
    assert po[0].msgctxt == "ui.start"
    assert po[0].msgid == "Start Game"
    assert po[0].msgstr == "Start Game"
    # Ensure original entry was not mutated
    assert entries["ui.start"].msgstr == ""


def test_main_cli_multi_file_and_flags(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch, capsys: pytest.CaptureFixture[str]
):
    src_dir = tmp_path / "src"
    src_dir.mkdir()

    # Luau file 1
    (src_dir / "view1.luau").write_text(
        "--@ Continue\n"
        'local btn = Tr("ui.continue")\n'
        "--@ Welcome, {user}!\n"
        'local msg = Tr("ui.welcome")\n'
    )

    # Lua file 2: shares "ui.continue" with exact same text
    (src_dir / "view2.lua").write_text(
        '--@ Continue\nbutton:OnClick(Tr("ui.continue"))\n'
    )

    # Non-lua file should be ignored
    (src_dir / "ignored.txt").write_text('--@ Ignored\nTr("ui.ignored")\n')

    out_pot = tmp_path / "out" / "serenkai.pot"

    monkeypatch.setattr(
        "sys.argv",
        ["extract_pot.py", "--src", str(src_dir), "--out", str(out_pot)],
    )

    main()

    captured = capsys.readouterr()
    assert "Done! Successfully extracted 2 entries" in captured.out
    assert captured.err == ""

    po = polib.pofile(str(out_pot))
    assert len(po) == 2

    by_ctxt = {e.msgctxt: e for e in po}

    # Verify ui.continue contains occurrences from both view1.luau and view2.lua
    continue_entry = by_ctxt["ui.continue"]
    assert len(continue_entry.occurrences) == 2

    # Verify ui.welcome has placeholder flag
    welcome_entry = by_ctxt["ui.welcome"]
    assert welcome_entry.msgid == "Welcome, {user}!"
    assert "python-brace-format" in welcome_entry.flags


def test_main_cli_conflict(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch, capsys: pytest.CaptureFixture[str]
):
    src_dir = tmp_path / "src"
    src_dir.mkdir()

    (src_dir / "file_a.luau").write_text('--@ Save game\nTr("ui.save")\n')
    (src_dir / "file_b.luau").write_text('--@ Save to disk\nTr("ui.save")\n')

    out_pot = tmp_path / "out" / "conflict.pot"

    monkeypatch.setattr(
        "sys.argv",
        ["extract_pot.py", "--src", str(src_dir), "--out", str(out_pot)],
    )

    main()

    captured = capsys.readouterr()
    assert "Key conflict for 'ui.save'" in captured.err
    assert "Found 1 key conflict(s)" in captured.err
    assert "Done! Successfully extracted 1 entries" in captured.out

    po = polib.pofile(str(out_pot))
    assert len(po) == 1
    assert len(po[0].occurrences) == 2


def test_main_cli_with_en_out(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch, capsys: pytest.CaptureFixture[str]
):
    src_dir = tmp_path / "src"
    src_dir.mkdir()

    (src_dir / "view.luau").write_text('--@ Exit Game\nTr("ui.exit")\n')

    out_pot = tmp_path / "out" / "template.pot"
    out_en_po = tmp_path / "out" / "en_US.po"

    monkeypatch.setattr(
        "sys.argv",
        [
            "extract_pot.py",
            "--src",
            str(src_dir),
            "--out",
            str(out_pot),
            "--en-out",
            str(out_en_po),
        ],
    )

    main()

    captured = capsys.readouterr()
    assert "Done! Successfully extracted 1 entries" in captured.out
    assert "Done! Successfully generated en_US PO file" in captured.out
    assert captured.err == ""

    # Verify template.pot keeps msgstr empty
    pot = polib.pofile(str(out_pot))
    assert len(pot) == 1
    assert pot[0].msgctxt == "ui.exit"
    assert pot[0].msgstr == ""
    assert "Language" not in pot.metadata

    # Verify en_US.po has msgstr populated with msgid and Language metadata
    po = polib.pofile(str(out_en_po))
    assert len(po) == 1
    assert po.metadata["Language"] == "en_US"
    assert po[0].msgctxt == "ui.exit"
    assert po[0].msgid == "Exit Game"
    assert po[0].msgstr == "Exit Game"
