#!/usr/bin/env python3
"""Derive Traditional Chinese (Taiwan) text from the Simplified Chinese sources.

- translations/catos-hardware-manager_zh_CN.ts -> _zh_TW.ts
- data/profiles/*.yaml: name_zh / description_zh -> name_zh_TW / description_zh_TW

Uses OpenCC's s2twp profile (Taiwan vocabulary). The result is machine
converted; a native reader should review it. Re-run after changing the
Simplified Chinese text: the zh_TW files are regenerated from scratch, so
put corrections into REPLACEMENTS below rather than editing them.
"""
import pathlib
import re
import subprocess
import sys
import xml.etree.ElementTree as ET

ROOT = pathlib.Path(__file__).resolve().parent.parent

# Fixes applied after OpenCC, for terms it leaves in mainland usage.
REPLACEMENTS = {
    "計算機": "電腦",
}


def convert(texts):
    """Convert many strings in one opencc call; LINE SEPARATOR keeps embedded newlines."""
    if not texts:
        return []
    joined = "\n".join(t.replace("\n", "\u2028") for t in texts)
    out = subprocess.run(["opencc", "-c", "s2twp"], input=joined, capture_output=True,
                         text=True, check=True).stdout
    lines = out.rstrip("\n").split("\n")
    if len(lines) != len(texts):
        sys.exit("opencc changed the number of lines")
    result = []
    for line in lines:
        for wrong, right in REPLACEMENTS.items():
            line = line.replace(wrong, right)
        result.append(line.replace("\u2028", "\n"))
    return result


def sync_ts():
    source = ROOT / "translations/catos-hardware-manager_zh_CN.ts"
    target = ROOT / "translations/catos-hardware-manager_zh_TW.ts"
    tree = ET.parse(source)
    root = tree.getroot()
    root.set("language", "zh_TW")
    slots = []
    for context in root.iter("context"):
        for message in context.iter("message"):
            translation = message.find("translation")
            nodes = list(translation.iter("numerusform")) or [translation]
            for node in nodes:
                if node.text:
                    slots.append(node)
    for node, text in zip(slots, convert([node.text for node in slots])):
        node.text = text
    ET.indent(root)
    target.write_text('<?xml version="1.0" encoding="utf-8"?>\n<!DOCTYPE TS>\n'
                      + ET.tostring(root, encoding="unicode") + "\n", encoding="utf-8")


def sync_profiles():
    for path in sorted((ROOT / "data/profiles").glob("*.yaml")):
        lines = path.read_text(encoding="utf-8").split("\n")
        keys = {}
        for line in lines:
            match = re.match(r"^(name_zh|description_zh):\s*(.*)$", line)
            if match:
                keys[match.group(1)] = match.group(2)
        if not keys:
            continue
        converted = dict(zip(keys, convert(list(keys.values()))))
        output = []
        for line in lines:
            if re.match(r"^(name_zh_TW|description_zh_TW):", line):
                continue
            output.append(line)
            match = re.match(r"^(name_zh|description_zh):", line)
            if match:
                output.append(f"{match.group(1)}_TW: {converted[match.group(1)]}")
        path.write_text("\n".join(output), encoding="utf-8")


if __name__ == "__main__":
    sync_ts()
    sync_profiles()
