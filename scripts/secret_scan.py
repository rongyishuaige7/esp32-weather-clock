#!/usr/bin/env python3
"""Fail publication checks when tracked content looks like local secrets or generated output."""
from __future__ import annotations

import argparse
import re
import subprocess
import sys
from pathlib import Path

FORBIDDEN_NAMES = {'.env', 'config.h', 'weather_clock_config.h', 'id_rsa', 'id_ed25519', 'local.properties'}
SKIP_DIRS = {'.git', '.pio', '.vscode', '.idea', 'build', 'dist', '__pycache__'}
FORBIDDEN_SUFFIXES = {'.o', '.a', '.elf', '.bin', '.map', '.hex', '.pyc', '.apk', '.aab', '.pem', '.key', '.zip', '.7z', '.tar', '.gz'}
TEXT_SUFFIXES = {'.md', '.py', '.sh', '.yml', '.yaml', '.json', '.jsonc', '.ini', '.h', '.cpp', '.svg', '.csv', '.txt', '.gitignore'}
PATTERNS = [
    ('private key block', re.compile(r'-----BEGIN [A-Z ]*PRIVATE KEY-----')),
    ('GitHub token', re.compile(r'\bgh[pousr]_[A-Za-z0-9_]{20,}\b')),
    ('OpenAI-style key', re.compile(r'\bsk-[A-Za-z0-9_-]{20,}\b')),
    ('private IPv4 address', re.compile(r'(?<![\d.])(?:10\.|192\.168\.(?!4\.1\b)|172\.(?:1[6-9]|2\d|3[01])\.)\d{1,3}\.\d{1,3}(?![\d.])')),
    ('absolute home path', re.compile(r'(?<![\w.-])/home/(?!rongyi/桌面/(?:esp32_weather_clock|esp32-weather-clock)\b)[^\s`"\']+')),
    ('suspicious QWeather key literal', re.compile(r'QWEATHER_API_KEY\s+"(?!")[A-Za-z0-9_-]{12,}"')),
    ('suspicious default Wi-Fi literal', re.compile(r'DEFAULT_WIFI_(?:SSID|PASSWORD)\s+"(?!")[^"\s]{4,}"')),
]
ALLOWED_PATH_FILES = {'docs/SOURCE_PROVENANCE.md'}


def tracked_files(root: Path) -> list[Path]:
    try:
        raw = subprocess.run(['git', '-C', str(root), 'ls-files', '-z'], check=True, capture_output=True).stdout
    except (FileNotFoundError, subprocess.CalledProcessError):
        raw = b''
    if raw:
        return [root / item.decode('utf-8', 'surrogateescape') for item in raw.split(b'\0') if item]
    return sorted(path for path in root.rglob('*') if path.is_file() and not any(part in SKIP_DIRS for part in path.relative_to(root).parts))


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument('--root', default='.')
    root = Path(parser.parse_args().root).resolve()
    errors: list[str] = []
    for path in tracked_files(root):
        rel = path.relative_to(root)
        if path.name in FORBIDDEN_NAMES:
            errors.append(f'{rel}: forbidden local/config file')
        if any(part in SKIP_DIRS for part in rel.parts):
            errors.append(f'{rel}: forbidden generated directory')
        if path.suffix.lower() in FORBIDDEN_SUFFIXES:
            errors.append(f'{rel}: forbidden binary/archive/key artifact')
        if path.stat().st_size > 5 * 1024 * 1024:
            errors.append(f'{rel}: file exceeds 5 MiB')
        if path.suffix.lower() not in TEXT_SUFFIXES or path.stat().st_size > 2_000_000:
            continue
        try:
            text = path.read_text(encoding='utf-8')
        except (OSError, UnicodeDecodeError):
            continue
        for number, line in enumerate(text.splitlines(), 1):
            for label, pattern in PATTERNS:
                if label == 'absolute home path' and rel.as_posix() in (ALLOWED_PATH_FILES | {'scripts/secret_scan.py'}):
                    continue
                if pattern.search(line):
                    errors.append(f'{rel}:{number}: {label}')
    if errors:
        print('Secret scan: FAIL', file=sys.stderr)
        print('\n'.join(sorted(set(errors))), file=sys.stderr)
        return 1
    print('Secret scan: PASS')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
