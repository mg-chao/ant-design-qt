#!/usr/bin/env python3
"""Synchronize Ant Design SVG icons into the default antd icon pack.

Default source: GitHub ant-design/ant-design-icons archive.
"""

from __future__ import annotations

import argparse
import datetime as _dt
import json
import re
import shutil
import tempfile
import urllib.request
import xml.etree.ElementTree as ET
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
RENDER_MODEL_ENUM = {
    "monochrome": "IconRenderModel::Monochrome",
    "twotone": "IconRenderModel::TwoTone",
    "threetone": "IconRenderModel::ThreeTone",
}
PRIMARY_COLORS = {"#333", "#333333", "#000", "#000000"}
SECONDARY_COLORS = {"#e6e6e6", "#d9d9d9", "#d8d8d8"}
TERTIARY_COLORS = {"#f5f5f5", "#f5f5f7"}
PACK_NAME = "antd"
SVG_NS = "http://www.w3.org/2000/svg"
ET.register_namespace("", SVG_NS)

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
    theme: str
    kebab_name: str
    function_name: str
    render_model: str
    qrc_alias: str
    qrc_path: str


def _download_bytes(url: str, timeout: int = 60) -> bytes:
    req = urllib.request.Request(url, headers={"User-Agent": "ant-design-icons-qt-sync/2.0"})
    with urllib.request.urlopen(req, timeout=timeout) as resp:
        return resp.read()


def _resolve_commit_sha(repo: str, ref: str) -> Optional[str]:
    api_url = f"https://api.github.com/repos/{repo}/commits/{ref}"
    try:
        payload = _download_bytes(api_url)
        data = json.loads(payload.decode("utf-8"))
    except Exception:
        return None
    sha = data.get("sha")
    return sha if isinstance(sha, str) and sha else None


def _resolve_default_branch(repo: str) -> Optional[str]:
    api_url = f"https://api.github.com/repos/{repo}"
    try:
        payload = _download_bytes(api_url)
        data = json.loads(payload.decode("utf-8"))
    except Exception:
        return None
    branch = data.get("default_branch")
    return branch if isinstance(branch, str) and branch else None


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

def _collect_local_svg_paths(local_root: Path) -> Dict[str, List[Path]]:
    result: Dict[str, List[Path]] = {theme: [] for theme in THEMES}
    for theme in THEMES:
        theme_dir = local_root / theme
        if not theme_dir.exists():
            continue
        if not theme_dir.is_dir():
            raise RuntimeError(f"Local icon theme path is not a directory: {theme_dir}")
        result[theme] = sorted(theme_dir.glob("*.svg"), key=lambda p: p.name)
    return result



def _local_name(tag: str) -> str:
    return tag.split("}", 1)[-1] if "}" in tag else tag


def _normalize_svg(svg_text: str, theme: str) -> Tuple[str, str]:
    root = ET.fromstring(svg_text)
    root.set("data-adqt-slot", "primary")
    if not root.get("fill"):
        root.set("fill", "currentColor")

    slots_used = {"primary"}

    for elem in root.iter():
        for attr_name in ("fill", "stroke"):
            value = elem.get(attr_name)
            if not value:
                continue
            lower = value.strip().lower()
            if lower in PRIMARY_COLORS:
                elem.set(attr_name, "currentColor")
            elif theme == "twotone" and lower in SECONDARY_COLORS:
                elem.set("data-adqt-slot", "secondary")
                slots_used.add("secondary")
            elif theme == "twotone" and lower in TERTIARY_COLORS:
                elem.set("data-adqt-slot", "tertiary")
                slots_used.add("tertiary")

    text = ET.tostring(root, encoding="unicode")
    text = text.replace(" />", "/>")
    text += "\n"

    if theme == "twotone":
        render_model = "threetone" if "tertiary" in slots_used else "twotone"
    else:
        render_model = "monochrome"
    return text, render_model


def _sync_svg_assets(
    svg_root: Path,
    dst_templates_root: Path,
    dry_run: bool,
    local_svg_paths: Optional[Dict[str, List[Path]]] = None,
) -> Dict[str, List[Tuple[str, str]]]:
    copied: Dict[str, List[Tuple[str, str]]] = {theme: [] for theme in THEMES}
    local_svg_paths = local_svg_paths or {theme: [] for theme in THEMES}

    for theme in THEMES:
        src_dir = svg_root / theme
        dst_dir = dst_templates_root / theme
        dst_dir.mkdir(parents=True, exist_ok=True)

        source_files: Dict[str, Path] = {}
        for src_file in sorted(src_dir.glob("*.svg"), key=lambda p: p.name):
            source_files[src_file.name] = src_file
        for src_file in local_svg_paths.get(theme, []):
            source_files[src_file.name] = src_file

        wanted_names = set(source_files.keys())
        for old_file in dst_dir.glob("*.svg"):
            if old_file.name not in wanted_names and not dry_run:
                old_file.unlink()

        for file_name in sorted(wanted_names):
            src_file = source_files[file_name]
            normalized_svg, render_model = _normalize_svg(src_file.read_text(encoding="utf-8"), theme)
            copied[theme].append((Path(file_name).stem, render_model))
            if dry_run:
                continue
            (dst_dir / file_name).write_text(normalized_svg, encoding="utf-8", newline="\n")

    return copied


def _build_entries(collected: Dict[str, List[Tuple[str, str]]]) -> List[Entry]:
    entries: List[Entry] = []
    for theme in THEMES:
        used: Dict[str, str] = {}
        for kebab_name, render_model in sorted(collected[theme], key=lambda item: item[0]):
            identifier = _pascal_identifier(kebab_name)
            prev = used.get(identifier)
            if prev is not None and prev != kebab_name:
                raise RuntimeError(
                    f"Identifier collision in theme '{theme}': '{prev}' and '{kebab_name}' both map to '{identifier}'."
                )
            used[identifier] = kebab_name
            entries.append(
                Entry(
                    theme=theme,
                    kebab_name=kebab_name,
                    function_name=identifier,
                    render_model=render_model,
                    qrc_alias=f"{theme}/{kebab_name}.svg",
                    qrc_path=f":/adqt/icons/{PACK_NAME}/{theme}/{kebab_name}.svg",
                )
            )
    return entries


def _render_qrc(entries: Sequence[Entry]) -> str:
    lines = ["<RCC>", f"  <qresource prefix=\"/adqt/icons/{PACK_NAME}\">"]
    for entry in entries:
        lines.append(
            f"    <file alias=\"{entry.qrc_alias}\">templates/{entry.theme}/{entry.kebab_name}.svg</file>"
        )
    lines.append("  </qresource>")
    lines.append("</RCC>")
    lines.append("")
    return "\n".join(lines)


def _render_pack_data_h() -> str:
    return """// Generated by tools/sync_ant_design_icons.py. DO NOT EDIT.
#ifndef ADQT_ANTD_PACK_DATA_H
#define ADQT_ANTD_PACK_DATA_H

#include \"ant_design_icons_qt_global.h\"
#include \"icon_core.h\"

namespace adqt::icons::antd::detail {

struct PackEntry {
  IconTheme theme = IconTheme::Outlined;
  IconRenderModel model = IconRenderModel::Monochrome;
  const char* name = \"\";
  const char* qrcPath = \"\";
};

ADQT_ICONS_EXPORT const PackEntry* packEntries();
ADQT_ICONS_EXPORT int packEntryCount();

}  // namespace adqt::icons::antd::detail

#endif  // ADQT_ANTD_PACK_DATA_H
"""


def _render_pack_data_cpp(entries: Sequence[Entry]) -> str:
    lines = [
        "// Generated by tools/sync_ant_design_icons.py. DO NOT EDIT.",
        "#include \"generated/antd_pack_data.h\"",
        "",
        "namespace adqt::icons::antd::detail {",
        "",
        "namespace {",
        "",
        "const PackEntry kEntries[] = {",
    ]
    for entry in entries:
        lines.append(
            f"    {{{THEME_ENUM[entry.theme]}, {RENDER_MODEL_ENUM[entry.render_model]}, \"{entry.kebab_name}\", \"{entry.qrc_path}\"}},"
        )
    lines += [
        "};",
        "",
        "}  // namespace",
        "",
        "const PackEntry* packEntries() {",
        "  return kEntries;",
        "}",
        "",
        "int packEntryCount() {",
        "  return static_cast<int>(sizeof(kEntries) / sizeof(kEntries[0]));",
        "}",
        "",
        "}  // namespace adqt::icons::antd::detail",
        "",
    ]
    return "\n".join(lines)


def _render_public_h(entries: Sequence[Entry]) -> str:
    grouped: Dict[str, List[Entry]] = {theme: [] for theme in THEMES}
    for entry in entries:
        grouped[entry.theme].append(entry)

    lines = [
        "// Generated by tools/sync_ant_design_icons.py. DO NOT EDIT.",
        "#ifndef ADQT_ANTD_ICONS_H",
        "#define ADQT_ANTD_ICONS_H",
        "",
        "#include \"ant_design_icons_qt_global.h\"",
        "#include \"icon_core.h\"",
        "#include \"version.h\"",
        "",
        "namespace adqt::icons::antd {",
        "",
        "ADQT_ICONS_EXPORT const char* version();",
        "ADQT_ICONS_EXPORT void registerAntdPack(IconRegistry& registry);",
        "ADQT_ICONS_EXPORT void ensureAntdRegistered();",
        "",
        "namespace outlined {",
    ]
    for entry in grouped["outlined"]:
        lines.append(f"ADQT_ICONS_EXPORT IconRef {entry.function_name}(const IconColorOverrides& colors = {{}});")
    lines += [
        "}  // namespace outlined",
        "",
        "namespace filled {",
    ]
    for entry in grouped["filled"]:
        lines.append(f"ADQT_ICONS_EXPORT IconRef {entry.function_name}(const IconColorOverrides& colors = {{}});")
    lines += [
        "}  // namespace filled",
        "",
        "namespace twotone {",
    ]
    for entry in grouped["twotone"]:
        lines.append(f"ADQT_ICONS_EXPORT IconRef {entry.function_name}(const IconColorOverrides& colors = {{}});")
    lines += [
        "}  // namespace twotone",
        "",
        "}  // namespace adqt::icons::antd",
        "",
        "#endif  // ADQT_ANTD_ICONS_H",
        "",
    ]
    return "\n".join(lines)


def _render_public_cpp(entries: Sequence[Entry]) -> str:
    grouped: Dict[str, List[Entry]] = {theme: [] for theme in THEMES}
    for entry in entries:
        grouped[entry.theme].append(entry)

    lines = [
        "// Generated by tools/sync_ant_design_icons.py. DO NOT EDIT.",
        "#include \"antd_icons.h\"",
        "",
        "#include \"generated/antd_pack_data.h\"",
        "",
        "#include <QFile>",
        "",
        "#include <mutex>",
        "",
        "int qInitResources_ant_design_icons();",
        "",
        "namespace adqt::icons::antd {",
        "namespace {",
        "",
        "void ensureResources() {",
        "  static bool initialized = false;",
        "  if (!initialized) {",
        "    ::qInitResources_ant_design_icons();",
        "    initialized = true;",
        "  }",
        "}",
        "",
        "IconRef makeRef(IconTheme theme, const char* name, const IconColorOverrides& colors) {",
        "  ensureAntdRegistered();",
        "  IconRef ref;",
        f"  ref.id.pack = QStringLiteral(\"{PACK_NAME}\");",
        "  ref.id.theme = theme;",
        "  ref.id.name = QString::fromLatin1(name);",
        "  ref.colors = colors;",
        "  return ref;",
        "}",
        "",
        "bool loadDefinition(const detail::PackEntry& entry, IconDefinition* definition) {",
        "  QFile file(QString::fromLatin1(entry.qrcPath));",
        "  if (!file.open(QIODevice::ReadOnly)) {",
        "    return false;",
        "  }",
        "  IconDefinition def;",
        f"  def.id.pack = QStringLiteral(\"{PACK_NAME}\");",
        "  def.id.theme = entry.theme;",
        "  def.id.name = QString::fromLatin1(entry.name);",
        "  def.model = entry.model;",
        "  def.svgTemplate = file.readAll();",
        "  if (!def.isValid()) {",
        "    return false;",
        "  }",
        "  *definition = def;",
        "  return true;",
        "}",
        "",
        "}  // namespace",
        "",
        "const char* version() {",
        "  return ADQT_ANTD_ICONS_VERSION_STR;",
        "}",
        "",
        "void registerAntdPack(IconRegistry& registry) {",
        "  ensureResources();",
        "  for (int index = 0; index < detail::packEntryCount(); ++index) {",
        "    IconDefinition definition;",
        "    if (loadDefinition(detail::packEntries()[index], &definition)) {",
        "      registry.registerIcon(definition);",
        "    }",
        "  }",
        "}",
        "",
        "void ensureAntdRegistered() {",
        "  static std::once_flag once;",
        "  std::call_once(once, []() { registerAntdPack(defaultRegistry()); });",
        "}",
        "",
        "namespace outlined {",
    ]
    for entry in grouped["outlined"]:
        lines += [
            f"IconRef {entry.function_name}(const IconColorOverrides& colors) {{",
            f"  return makeRef(IconTheme::Outlined, \"{entry.kebab_name}\", colors);",
            "}",
            "",
        ]
    lines += [
        "}  // namespace outlined",
        "",
        "namespace filled {",
    ]
    for entry in grouped["filled"]:
        lines += [
            f"IconRef {entry.function_name}(const IconColorOverrides& colors) {{",
            f"  return makeRef(IconTheme::Filled, \"{entry.kebab_name}\", colors);",
            "}",
            "",
        ]
    lines += [
        "}  // namespace filled",
        "",
        "namespace twotone {",
    ]
    for entry in grouped["twotone"]:
        lines += [
            f"IconRef {entry.function_name}(const IconColorOverrides& colors) {{",
            f"  return makeRef(IconTheme::TwoTone, \"{entry.kebab_name}\", colors);",
            "}",
            "",
        ]
    lines += [
        "}  // namespace twotone",
        "",
        "}  // namespace adqt::icons::antd",
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
    parser = argparse.ArgumentParser(description="Sync Ant Design SVG assets into the default antd pack.")
    parser.add_argument("--repo", default="ant-design/ant-design-icons", help="GitHub repo in owner/name format.")
    parser.add_argument("--ref", default="main", help="Git ref (default: main).")
    parser.add_argument("--dry-run", action="store_true", help="Print summary without writing files.")
    args = parser.parse_args(argv)

    root = Path(__file__).resolve().parents[1]
    pkg_root = root / "packages" / "ant-design-icons-qt"
    resources_root = pkg_root / "resources"
    templates_root = resources_root / "templates"
    local_icons_root = resources_root / "custom-icons"
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
        local_svg_paths = _collect_local_svg_paths(local_icons_root)
        collected_names = _sync_svg_assets(svg_root, templates_root, args.dry_run, local_svg_paths=local_svg_paths)
        entries = _build_entries(collected_names)

        _write_text(resources_root / "ant_design_icons.qrc", _render_qrc(entries), args.dry_run)
        _write_text(pkg_root / "src" / "generated" / "antd_pack_data.h", _render_pack_data_h(), args.dry_run)
        _write_text(pkg_root / "src" / "generated" / "antd_pack_data.cpp", _render_pack_data_cpp(entries), args.dry_run)
        _write_text(pkg_root / "src" / "antd_icons.h", _render_public_h(entries), args.dry_run)
        _write_text(pkg_root / "src" / "antd_icons.cpp", _render_public_cpp(entries), args.dry_run)

        lock_payload = {
            "repository": args.repo,
            "ref": args.ref,
            "resolved_ref": used_ref,
            "archive_url": used_url,
            "resolved_commit": _resolve_commit_sha(args.repo, used_ref),
            "generated_at_utc": _dt.datetime.now(_dt.timezone.utc).replace(microsecond=0).isoformat().replace("+00:00", "Z"),
            "counts": {
                "outlined": len(collected_names["outlined"]),
                "filled": len(collected_names["filled"]),
                "twotone": len(collected_names["twotone"]),
                "total": len(entries),
            },
        }
        _write_json(resources_root / "upstream.lock.json", lock_payload, args.dry_run)

    print(
        f"synced outlined={len(collected_names['outlined'])} filled={len(collected_names['filled'])} "
        f"twotone={len(collected_names['twotone'])} total={len(entries)} dry_run={args.dry_run}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
