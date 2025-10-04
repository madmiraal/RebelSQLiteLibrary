#!/usr/bin/env bash
# SPDX-FileCopyrightText: 2025 Rebel SQLite Library contributors
# SPDX-FileCopyrightText: 2022 Rebel Engine contributors
# SPDX-FileCopyrightText: 2019-2022 Godot Engine contributors
#
# SPDX-License-Identifier: MIT

# This script ensures that all text files:
# 1. are UTF-8 encoded,
# 2. do not contain a BOM,
# 3. have LF line endings and line endings do not have trailing white,
# 4. do not use a superflous '== true'.

# Check for uncommitted changes.
if [[ $(git diff) ]]; then
    echo "You have uncommitted changes!"
    echo "Please commit, stage or stash your changes and try again."
    exit 1
fi

# Check if recode is installed.
if ! command -v recode > /dev/null; then
	echo "Error: 'recode' executable not found."
    echo "Please install recode, and try again."
	exit 1
fi

# Check if dos2unix is installed.
if ! command -v dos2unix > /dev/null; then
	echo "Error: 'dos2unix' executable not found."
    echo "Please install dos2unix, and try again."
	exit 1
fi

# Check if perl is installed.
if ! command -v perl > /dev/null; then
	echo "Error: 'perl' executable not found."
    echo "Please install perl, and try again."
	exit 1
fi

set -uo pipefail

echo "Formatting all text files..."
IFS=$'\n\t'
# Loops through all text files tracked by Git.
git grep -zIl '' |
while IFS= read -rd '' file; do
    # Exclude RebelSDK submodule.
    if [[ "$file" == "RebelSDK/"* ]]; then
        continue
    # Exclued third-party code.
    elif [[ "$file" == "third-party/"* ]]; then
        continue
    fi

    # Ensure that files are UTF-8 formatted.
    recode UTF-8 "$file" 2> /dev/null
    # Ensure that files have LF line endings and do not contain a BOM.
    dos2unix "$file" 2> /dev/null
    # Remove trailing space characters and
    # ensure that files end with newline characters.
    # -l option handles newlines conveniently.
    perl -i -ple 's/\s*$//g' "$file"
    # Remove the character sequence "== true" if it has a leading space.
    perl -i -pe 's/\x20== true//g' "$file"
done

# Check if a diff has been created.
if [[ $(git diff) ]]; then
    echo "Files in this commit do not comply with the file formatting rules."
    echo "The following changes need to be made:"
    echo
    git diff --color
    echo
    echo "Please fix your commit."
    exit 1
fi

echo "Files in this commit comply with the file formatting rules."
