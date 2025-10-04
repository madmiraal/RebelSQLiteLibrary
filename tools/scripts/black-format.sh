#!/usr/bin/env bash
# SPDX-FileCopyrightText: 2025 Rebel SQLite Library contributors
# SPDX-FileCopyrightText: 2022 Rebel Engine contributors
# SPDX-FileCopyrightText: 2019-2022 Godot Engine contributors
#
# SPDX-License-Identifier: MIT

# This script applies black to all Python files.

# Check for uncommitted changes.
if [[ $(git diff) ]]; then
    echo "You have uncommitted changes!"
    echo "Please commit, stage or stash your changes and try again."
    exit 1
fi

# Check if black is installed.
if ! command -v black > /dev/null; then
	echo "Error: 'black' executable not found."
    echo "Please install black, and try again."
	exit 1
fi

set -uo pipefail

echo "Formatting Python files..."
PY_FILES=$(find \( -path "./.git" \
                -o -path "./RebelSDK" \
                -o -path "./third-party" \
                \) -prune \
                -o \( -name "SConstruct" \
                -o -name "*.py" \
                \) -print)
# Apply black.
black $PY_FILES

# Check if a diff has been created.
if [[ $(git diff) ]]; then
    echo "Files in this commit do not comply with the black Python style rules."
    echo "The following changes need to be made:"
    echo
    git diff --color
    echo
    echo "Please fix your commit."
    exit 1
fi

echo "Files in this commit comply with the black Python style rules."
