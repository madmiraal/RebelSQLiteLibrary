#!/usr/bin/env bash
# SPDX-FileCopyrightText: 2025 Rebel SQLite Library contributors
# SPDX-FileCopyrightText: 2022 Rebel Engine contributors
# SPDX-FileCopyrightText: 2019-2022 Godot Engine contributors
#
# SPDX-License-Identifier: MIT

# This script applies clang-format to all code files.
# The `clang-format` rules are defined in `.clang-format` in the root directory.
# The version of `clang-format` used affects the output generated.
# To get consistent formatting, use the same clang-format version as the Github
# workflow style tests: currently version 18.

# Check for uncommitted changes.
if [[ $(git diff) ]]; then
    echo "You have uncommitted changes!"
    echo "Please commit, stage or stash your changes and try again."
    exit 1
fi

# Check if clang-format is installed.
if ! command -v clang-format > /dev/null; then
	echo "Error: 'clang-format' executable not found."
    echo "Please install clang-format, and try again."
	exit 1
fi

set -uo pipefail

echo "Applying clang-format to all code files..."
IFS=$'\n\t'
FILE_EXTS=(".h" ".c" ".cpp")
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

    for extension in ${FILE_EXTS[@]}; do
        if [[ "$file" == *"$extension" ]]; then
            # Run clang-format.
            clang-format --Wno-error=unknown -i "$file"
            continue 2
        fi
    done
done

# Check if a diff has been created.
if [[ $(git diff) ]]; then
    echo "Files in this commit do not comply with the clang-format style rules."
    echo "The following changes need to be made:"
    echo
    git diff --color
    echo
    echo "Please fix your commit."
    exit 1
fi

echo "Files in this commit comply with the clang-format style rules."
