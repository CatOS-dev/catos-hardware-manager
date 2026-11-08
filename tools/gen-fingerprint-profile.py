#!/usr/bin/env python3
"""Generate a libfprint support profile from source hwdb-like files."""
from pathlib import Path
import re
import sys

if len(sys.argv) != 3:
    raise SystemExit(f"usage: {sys.argv[0]} <libfprint-source> <output-yaml>")
source = Path(sys.argv[1])
output = Path(sys.argv[2])
text = source.read_text(encoding="utf-8")
pairs = {(v.lower(), p.lower()) for v, p in re.findall(
    r"^usb:v([0-9a-fA-F]{4})p([0-9a-fA-F]{4})\*$", text, re.MULTILINE)}
pairs.update((v.lower(), p.lower()) for v, p in re.findall(
    r"\b(?:USB_DEVICE|USB_DEVICE_ID)\s*\(\s*0x([0-9a-fA-F]{4})\s*,\s*0x([0-9a-fA-F]{4})\s*\)", text))
pairs.update((v.lower(), p.lower()) for v, p in re.findall(
    r"\.vendor_id\s*=\s*0x([0-9a-fA-F]{4}).{0,120}?\.product_id\s*=\s*0x([0-9a-fA-F]{4})", text, re.DOTALL))
pairs = sorted(pairs)
if not pairs:
    raise SystemExit("no USB vendor/product IDs found")
with output.open("w", encoding="utf-8") as fh:
    fh.write("id: fingerprint\nname: Fingerprint reader support\nname_zh: 指纹识别支持\n")
    fh.write("description: fprintd and libfprint\ndescription_zh: fprintd 与 libfprint 指纹服务\n")
    fh.write("category: fingerprint\nrecommended: true\npackages: [fprintd]\nmatch:\n  any:\n")
    for vendor, product in pairs:
        fh.write(f'    - bus: usb\n      vendor: "{vendor}"\n      device: "{product}"\n')
