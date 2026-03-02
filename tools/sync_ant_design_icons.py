#!/usr/bin/env python3
"""Synchronize ant-design-icons SVG assets into ant-design-icons-qt.

Default source: GitHub ant-design/ant-design-icons main branch archive.
"""

from __future__ import annotations

import argparse
import datetime as _dt
import json
import re
import shutil
import tempfile
import urllib.request
import zipfile
from dataclasses import dataclass
from pathlib import Path
from typing import Dict, List, Optional, Sequence, Tuple

THEMES: Tuple[str, ...] = ("outlined", "filled", "twotone")
THEME_ENUM = {
    "outlined": "IconTheme::Outlined",
    "filled": "IconTheme::Filled",
    "twotone": "IconTheme::TwoTone",
}

KEYWORDS = {
    "alignas", "alignof", "and", "and_eq", "asm", "auto", "bitand", "bitor", "bool", "break",
    "case", "catch", "char", "char8_t", "char16_t", "char32_t", "class", "compl", "concept",
    "const", "consteval", "constexpr", "constinit", "const_cast", "continue", "co_await", "co_return",
    "co_yield", "decltype", "default", "delete", "do", "double", "dynamic_cast", "else", "enum",
    "explicit", "export", "extern", "false", "float", "for", "friend", "goto", "if", "inline", "int",
    "long", "mutable", "namespace", "new", "noexcept", "not", "not_eq", "nullptr", "operator", "or",
    "or_eq", "private", "protected", "public", "register", "reinterpret_cast", "requires", "return",
    "short", "signed", "sizeof", "static", "static_assert", "static_cast", "struct", "switch", "template",
    "this", "thread_local", "throw", "true", "try", "typedef", "typeid", "typename", "union", "unsigned",
    "using", "virtual", "void", "volatile", "wchar_t", "while", "xor", "xor_eq",
}


@dataclass(frozen=True)
class Entry:
    index: int
    theme: str
    kebab_name: str
    function_name: str
    qrc_alias: str
    qrc_path: str


def _download_bytes(url: str, timeout: int = 60) -> bytes:
    req = urllib.request.Request(url, headers={"User-Agent": "ant-design-icons-qt-sync/1.0"})
    with urllib.request.urlopen(req, timeout=timeout) as resp:
        return resp.read()


def _resolve_commit_sha(repo: str, ref: str) -> Optional[str]:
    api_url = f"https://api.github.com/repos/{repo}/commits/{ref}"
    try:
        payload = _download_bytes(api_url)
    except Exception:
        return None

    try:
        data = json.loads(payload.decode("utf-8"))
    except Exception:
        return None

    sha = data.get("sha")
    if isinstance(sha, str) and sha:
        return sha
    return None


def _resolve_default_branch(repo: str) -> Optional[str]:
    api_url = f"https://api.github.com/repos/{repo}"
    try:
        payload = _download_bytes(api_url)
        data = json.loads(payload.decode("utf-8"))
    except Exception:
        return None
    branch = data.get("default_branch")
    if isinstance(branch, str) and branch:
        return branch
    return None


def _download_archive(repo: str, ref: str, out_zip: Path) -> Tuple[str, str]:
    refs_to_try: List[str] = [ref]
    if ref == "main":
        refs_to_try.append("master")

    default_branch = _resolve_default_branch(repo)
    if default_branch and default_branch not in refs_to_try:
        refs_to_try.append(default_branch)

    candidates = [
        (candidate_ref, f"https://codeload.github.com/{repo}/zip/refs/heads/{candidate_ref}")
        for candidate_ref in refs_to_try
    ]
    candidates += [
        (candidate_ref, f"https://codeload.github.com/{repo}/zip/refs/tags/{candidate_ref}")
        for candidate_ref in refs_to_try
    ]
    candidates += [
        (candidate_ref, f"https://codeload.github.com/{repo}/zip/{candidate_ref}")
        for candidate_ref in refs_to_try
    ]

    last_error: Optional[Exception] = None
    for used_ref, url in candidates:
        try:
            payload = _download_bytes(url)
            out_zip.write_bytes(payload)
            return used_ref, url
        except Exception as exc:
            last_error = exc

    raise RuntimeError(f"Failed to download archive for ref '{ref}': {last_error}")


def _extract_archive(zip_path: Path, dst: Path) -> Path:
    with zipfile.ZipFile(zip_path, "r") as zf:
        zf.extractall(dst)

    roots = [p for p in dst.iterdir() if p.is_dir()]
    if len(roots) != 1:
        raise RuntimeError(f"Expected exactly one extracted root directory, got {len(roots)}.")
    return roots[0]


def _pascal_identifier(kebab_name: str) -> str:
    parts = re.findall(r"[A-Za-z0-9]+", kebab_name)
    if not parts:
        raise RuntimeError(f"Cannot derive C++ identifier from '{kebab_name}'.")

    identifier = "".join(part[0].upper() + part[1:] for part in parts)
    if identifier[0].isdigit():
        identifier = "Icon" + identifier
    if identifier in KEYWORDS:
        identifier = "Icon" + identifier[0].upper() + identifier[1:]
    return identifier


def _collect_svg_paths(svg_root: Path) -> Dict[str, List[Path]]:
    result: Dict[str, List[Path]] = {}
    for theme in THEMES:
        theme_dir = svg_root / theme
        if not theme_dir.exists() or not theme_dir.is_dir():
            raise RuntimeError(f"Missing svg theme directory: {theme_dir}")
        result[theme] = sorted(theme_dir.glob("*.svg"), key=lambda p: p.name)
    return result


def _sync_svg_assets(svg_root: Path, dst_icons_root: Path, dry_run: bool) -> Dict[str, List[str]]:
    copied: Dict[str, List[str]] = {theme: [] for theme in THEMES}

    for theme in THEMES:
        src_dir = svg_root / theme
        dst_dir = dst_icons_root / theme
        dst_dir.mkdir(parents=True, exist_ok=True)

        existing = sorted(dst_dir.glob("*.svg"))
        wanted_names = {p.name for p in src_dir.glob("*.svg")}

        for old_file in existing:
            if old_file.name not in wanted_names and not dry_run:
                old_file.unlink()

        for src_file in sorted(src_dir.glob("*.svg"), key=lambda p: p.name):
            dst_file = dst_dir / src_file.name
            copied[theme].append(src_file.stem)
            if dry_run:
                continue
            shutil.copy2(src_file, dst_file)

    return copied


def _build_entries(collected_names: Dict[str, List[str]]) -> List[Entry]:
    entries: List[Entry] = []
    index = 0

    for theme in THEMES:
        used: Dict[str, str] = {}
        for kebab_name in sorted(collected_names[theme]):
            identifier = _pascal_identifier(kebab_name)
            prev = used.get(identifier)
            if prev is not None and prev != kebab_name:
                raise RuntimeError(
                    f"Identifier collision in theme '{theme}': '{prev}' and '{kebab_name}' both map to '{identifier}'."
                )
            used[identifier] = kebab_name
            entries.append(
                Entry(
                    index=index,
                    theme=theme,
                    kebab_name=kebab_name,
                    function_name=identifier,
                    qrc_alias=f"{theme}/{kebab_name}.svg",
                    qrc_path=f":/adqt/icons/{theme}/{kebab_name}.svg",
                )
            )
            index += 1

    return entries


def _render_qrc(entries: Sequence[Entry]) -> str:
    lines = ["<RCC>", "  <qresource prefix=\"/adqt/icons\">"]
    for entry in entries:
        lines.append(
            f"    <file alias=\"{entry.qrc_alias}\">icons/{entry.theme}/{entry.kebab_name}.svg</file>"
        )
    lines.append("  </qresource>")
    lines.append("</RCC>")
    lines.append("")
    return "\n".join(lines)


def _render_manifest_h() -> str:
    return """// Generated by tools/sync_ant_design_icons.py. DO NOT EDIT.
#ifndef ADQT_ICONS_GENERATED_ICON_MANIFEST_H
#define ADQT_ICONS_GENERATED_ICON_MANIFEST_H

#include "ant_design_icons_qt_global.h"
#include "icons_types.h"

namespace adqt::icons::detail {

struct IconEntry {
  IconTheme theme = IconTheme::Outlined;
  const char* name = "";
  const char* qrcPath = "";
};

ADQT_ICONS_EXPORT const IconEntry* iconEntries();
ADQT_ICONS_EXPORT int iconEntryCount();
ADQT_ICONS_EXPORT const IconEntry& iconEntryAt(int index);

}  // namespace adqt::icons::detail

#endif  // ADQT_ICONS_GENERATED_ICON_MANIFEST_H
"""


def _render_manifest_cpp(entries: Sequence[Entry]) -> str:
    lines = [
        "// Generated by tools/sync_ant_design_icons.py. DO NOT EDIT.",
        "#include \"generated/icon_manifest.h\"",
        "",
        "namespace adqt::icons::detail {",
        "",
        "namespace {",
        "",
        "const IconEntry kEntries[] = {",
    ]

    for e in entries:
        lines.append(f"    {{{THEME_ENUM[e.theme]}, \"{e.kebab_name}\", \"{e.qrc_path}\"}},")

    lines += [
        "};",
        "",
        "const IconEntry kInvalidEntry = {",
        "    IconTheme::Outlined,",
        "    \"\",",
        "    \"\",",
        "};",
        "",
        "}  // namespace",
        "",
        "const IconEntry* iconEntries() {",
        "  return kEntries;",
        "}",
        "",
        "int iconEntryCount() {",
        "  return static_cast<int>(sizeof(kEntries) / sizeof(kEntries[0]));",
        "}",
        "",
        "const IconEntry& iconEntryAt(int index) {",
        "  if (index < 0 || index >= iconEntryCount()) {",
        "    return kInvalidEntry;",
        "  }",
        "  return kEntries[index];",
        "}",
        "",
        "}  // namespace adqt::icons::detail",
        "",
    ]

    return "\n".join(lines)


def _render_functions_h(entries: Sequence[Entry]) -> str:
    grouped: Dict[str, List[Entry]] = {theme: [] for theme in THEMES}
    for e in entries:
        grouped[e.theme].append(e)

    lines = [
        "// Generated by tools/sync_ant_design_icons.py. DO NOT EDIT.",
        "#ifndef ADQT_ICONS_GENERATED_ICON_FUNCTIONS_H",
        "#define ADQT_ICONS_GENERATED_ICON_FUNCTIONS_H",
        "",
        "#include \"ant_design_icons_qt_global.h\"",
        "#include \"icons_types.h\"",
        "",
        "namespace adqt::icons::outlined {",
    ]
    for e in grouped["outlined"]:
        lines.append(f"ADQT_ICONS_EXPORT IconToken {e.function_name}(const IconStyle& style = {{}});")

    lines += [
        "}  // namespace adqt::icons::outlined",
        "",
        "namespace adqt::icons::filled {",
    ]
    for e in grouped["filled"]:
        lines.append(f"ADQT_ICONS_EXPORT IconToken {e.function_name}(const IconStyle& style = {{}});")

    lines += [
        "}  // namespace adqt::icons::filled",
        "",
        "namespace adqt::icons::twotone {",
    ]
    for e in grouped["twotone"]:
        lines.append(f"ADQT_ICONS_EXPORT IconToken {e.function_name}(const IconStyle& style = {{}});")

    lines += [
        "}  // namespace adqt::icons::twotone",
        "",
        "#endif  // ADQT_ICONS_GENERATED_ICON_FUNCTIONS_H",
        "",
    ]

    return "\n".join(lines)


def _render_functions_cpp(entries: Sequence[Entry]) -> str:
    grouped: Dict[str, List[Entry]] = {theme: [] for theme in THEMES}
    for e in entries:
        grouped[e.theme].append(e)

    lines = [
        "// Generated by tools/sync_ant_design_icons.py. DO NOT EDIT.",
        "#include \"generated/icon_functions.h\"",
        "",
        "#include \"icon_provider.h\"",
        "",
        "namespace adqt::icons::outlined {",
    ]

    for e in grouped["outlined"]:
        lines += [
            f"IconToken {e.function_name}(const IconStyle& style) {{",
            f"  return detail::makeTokenByIndex({e.index}, style);",
            "}",
            "",
        ]

    lines += [
        "}  // namespace adqt::icons::outlined",
        "",
        "namespace adqt::icons::filled {",
    ]

    for e in grouped["filled"]:
        lines += [
            f"IconToken {e.function_name}(const IconStyle& style) {{",
            f"  return detail::makeTokenByIndex({e.index}, style);",
            "}",
            "",
        ]

    lines += [
        "}  // namespace adqt::icons::filled",
        "",
        "namespace adqt::icons::twotone {",
    ]

    for e in grouped["twotone"]:
        lines += [
            f"IconToken {e.function_name}(const IconStyle& style) {{",
            f"  return detail::makeTokenByIndex({e.index}, style);",
            "}",
            "",
        ]

    lines += [
        "}  // namespace adqt::icons::twotone",
        "",
    ]

    return "\n".join(lines)


def _write_text(path: Path, content: str, dry_run: bool) -> None:
    if dry_run:
        return
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(content, encoding="utf-8", newline="\n")


def _write_json(path: Path, payload: dict, dry_run: bool) -> None:
    if dry_run:
        return
    path.parent.mkdir(parents=True, exist_ok=True)
    text = json.dumps(payload, ensure_ascii=False, indent=2)
    path.write_text(text + "\n", encoding="utf-8", newline="\n")


def main(argv: Optional[Sequence[str]] = None) -> int:
    parser = argparse.ArgumentParser(description="Sync ant-design-icons SVG assets into ant-design-icons-qt.")
    parser.add_argument("--repo", default="ant-design/ant-design-icons", help="GitHub repo in owner/name format.")
    parser.add_argument("--ref", default="main", help="Git ref (default: main).")
    parser.add_argument("--dry-run", action="store_true", help="Print summary without writing files.")
    args = parser.parse_args(argv)

    root = Path(__file__).resolve().parents[1]
    pkg_root = root / "packages" / "ant-design-icons-qt"
    resources_root = pkg_root / "resources"
    icons_root = resources_root / "icons"
    generated_root = pkg_root / "src" / "generated"

    if not pkg_root.exists():
        raise RuntimeError(f"Package path not found: {pkg_root}")

    with tempfile.TemporaryDirectory(prefix="ant-design-icons-sync-") as tmp:
        tmp_path = Path(tmp)
        archive = tmp_path / "upstream.zip"

        used_ref, used_url = _download_archive(args.repo, args.ref, archive)
        extracted_root = _extract_archive(archive, tmp_path / "src")
        svg_root = extracted_root / "packages" / "icons-svg" / "svg"

        _collect_svg_paths(svg_root)
        copied_names = _sync_svg_assets(svg_root, icons_root, args.dry_run)

        entries = _build_entries(copied_names)

        _write_text(resources_root / "ant_design_icons.qrc", _render_qrc(entries), args.dry_run)
        _write_text(generated_root / "icon_manifest.h", _render_manifest_h(), args.dry_run)
        _write_text(generated_root / "icon_manifest.cpp", _render_manifest_cpp(entries), args.dry_run)
        _write_text(generated_root / "icon_functions.h", _render_functions_h(entries), args.dry_run)
        _write_text(generated_root / "icon_functions.cpp", _render_functions_cpp(entries), args.dry_run)

        lock_payload = {
            "repository": args.repo,
            "ref": args.ref,
            "resolved_ref": used_ref,
            "archive_url": used_url,
            "resolved_commit": _resolve_commit_sha(args.repo, used_ref),
            "generated_at_utc": _dt.datetime.now(_dt.timezone.utc).replace(microsecond=0).isoformat().replace("+00:00", "Z"),
            "counts": {
                "outlined": len(copied_names["outlined"]),
                "filled": len(copied_names["filled"]),
                "twotone": len(copied_names["twotone"]),
                "total": len(entries),
            },
        }
        _write_json(resources_root / "upstream.lock.json", lock_payload, args.dry_run)

    print(
        f"synced outlined={len(copied_names['outlined'])} filled={len(copied_names['filled'])} "
        f"twotone={len(copied_names['twotone'])} total={len(entries)} dry_run={args.dry_run}"
    )

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
