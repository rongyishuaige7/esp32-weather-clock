#!/usr/bin/env python3
"""Validate public-repository contracts that require no physical hardware."""
from __future__ import annotations

import argparse
import csv
import subprocess
import sys
import xml.etree.ElementTree as ET
from pathlib import Path

REQUIRED = [
    '.github/platformio-requirements.in', '.github/platformio-requirements.txt', '.github/workflows/validate.yml',
    '.gitignore', '.markdownlint-cli2.jsonc', 'HARDWARE.md', 'LICENSE', 'README.md', 'SECURITY.md',
    'THIRD_PARTY_NOTICES.md', 'docs/GITHUB_METADATA.md', 'docs/HARDWARE_LAB_CARD.md',
    'docs/PROJECT_STATUS.md', 'docs/PROTOCOL.md', 'docs/SOURCE_PROVENANCE.md', 'docs/VERIFICATION.md',
    'firmware/include/weather_clock_config.example.h', 'firmware/include/wifi_config_html.h', 'firmware/platformio.ini',
    'firmware/src/main.cpp', 'hardware/BOM.csv', 'hardware/wiring-diagram.svg', 'scripts/check_repo.py',
    'scripts/secret_scan.py', 'scripts/verify.sh', 'tests/test_source_contracts.py',
]
FORBIDDEN_NAMES = {'.env', 'weather_clock_config.h', 'id_rsa', 'id_ed25519', 'local.properties'}
FORBIDDEN_DIRS = {'.git', '.pio', '.vscode', '.idea', 'build', 'dist', '__pycache__'}
FORBIDDEN_SUFFIXES = {'.o', '.a', '.elf', '.bin', '.map', '.hex', '.pyc', '.apk', '.aab', '.pem', '.key', '.zip', '.7z', '.tar', '.gz'}


def files(root: Path) -> list[Path]:
    try:
        raw = subprocess.run(['git', '-C', str(root), 'ls-files', '-z'], check=True, capture_output=True).stdout
    except (subprocess.CalledProcessError, FileNotFoundError):
        raw = b''
    if raw:
        return [root / item.decode('utf-8', 'surrogateescape') for item in raw.split(b'\0') if item]
    return sorted(path for path in root.rglob('*') if path.is_file() and not any(part in FORBIDDEN_DIRS for part in path.relative_to(root).parts))


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument('--root', default='.')
    root = Path(parser.parse_args().root).resolve()
    errors: list[str] = []
    for rel in REQUIRED:
        if not (root / rel).is_file():
            errors.append(f'missing required file: {rel}')
    if (root / 'firmware/include/weather_clock_config.h').exists():
        errors.append('firmware/include/weather_clock_config.h must not exist in public candidate')
    checked = files(root)
    for path in checked:
        rel = path.relative_to(root)
        if path.name in FORBIDDEN_NAMES:
            errors.append(f'forbidden local/config file: {rel}')
        if any(part in FORBIDDEN_DIRS for part in rel.parts):
            errors.append(f'forbidden generated directory: {rel}')
        if path.suffix.lower() in FORBIDDEN_SUFFIXES:
            errors.append(f'forbidden binary/archive/key artifact: {rel}')
        if path.stat().st_size > 5 * 1024 * 1024:
            errors.append(f'file exceeds 5 MiB: {rel}')

    contracts = {
        'firmware/platformio.ini': ['platform = espressif32@6.13.0', 'board = esp32dev', 'rzeldent/micro-miniz@1.0.0'],
        'firmware/include/weather_clock_config.example.h': ['#define QWEATHER_API_KEY ""', '#define DEFAULT_WIFI_SSID ""', '#define AP_SSID "WeatherClock-Setup"'],
        'firmware/src/main.cpp': [
            '#if __has_include("weather_clock_config.h")', '#include "weather_clock_config.example.h"', 'WiFi.softAP(AP_SSID);',
            'client.setInsecure();', 'server->send(404, "application/json", "{\\"error\\":\\"not_found\\"}");',
            'String ssid = doc["ssid"] | "";', 'String apikey = doc["apikey"] | "";',
        ],
        'firmware/include/wifi_config_html.h': ['value=""', '设备不会回显已保存的 Key', 'var CITIES='],
        'docs/SOURCE_PROVENANCE.md': [
            '09f3363acd82b8c1f82e6fede2047752621e57fc1fc67682b004cd8af6c0006f',
            '32b631e0b0b9c15e7cc06771e456b948fa033f05985d440f858408565c226236',
        ],
        'README.md': ['无认证、无 TLS', 'setInsecure()', 'Flash'],
    }
    for rel, values in contracts.items():
        path = root / rel
        if not path.is_file():
            continue
        text = path.read_text(encoding='utf-8')
        for value in values:
            if value not in text:
                errors.append(f'fact contract missing in {rel}: {value}')

    main_cpp = (root / 'firmware/src/main.cpp').read_text(encoding='utf-8') if (root / 'firmware/src/main.cpp').is_file() else ''
    html = (root / 'firmware/include/wifi_config_html.h').read_text(encoding='utf-8') if (root / 'firmware/include/wifi_config_html.h').is_file() else ''
    for forbidden in ['AP_PASSWORD', 'server.sendContent(apiKey)', 'Serial.println("SSID: "', 'Serial.println("API Key: "', 'WiFi.softAP(AP_SSID,']:
        if forbidden in main_cpp:
            errors.append(f'public firmware contains forbidden secret/credential behavior: {forbidden}')
    if 'fonts.googleapis.com' in html or 'QWEATHER_DEFAULT_KEY' in html:
        errors.append('configuration HTML contains external font or injected API-key placeholder')

    for rel in ['README.md', 'docs/PROJECT_STATUS.md', 'docs/HARDWARE_LAB_CARD.md']:
        path = root / rel
        text = path.read_text(encoding='utf-8').lower() if path.is_file() else ''
        for claim in ['system online', 'current hardware verified', 'production ready']:
            if claim in text:
                errors.append(f'unsupported claim in {rel}: {claim}')
    try:
        ET.parse(root / 'hardware/wiring-diagram.svg')
    except (ET.ParseError, OSError) as exc:
        errors.append(f'invalid wiring SVG: {exc}')
    try:
        rows = list(csv.DictReader((root / 'hardware/BOM.csv').open(newline='', encoding='utf-8')))
        if len(rows) < 8:
            errors.append('BOM must contain at least 8 component rows')
    except (OSError, csv.Error) as exc:
        errors.append(f'invalid BOM.csv: {exc}')
    if errors:
        print('Repository check: FAIL', file=sys.stderr)
        for item in sorted(set(errors)):
            print(f'- {item}', file=sys.stderr)
        return 1
    print(f'Repository check: PASS ({len(checked)} files checked)')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
