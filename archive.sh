#!/bin/sh
# SPDX-FileCopyrightText: © 2017-2026 Adrian Johnston.
# SPDX-License-Identifier: MIT
# This file is licensed under the terms of the LICENSE.md file.
#
# This script only archives the .git folder as it contains everything needed to
# restore the full file tree of the current commit (and all previous).

TOP_LEVEL="$(cd "$(dirname "$0")" && pwd)"

if [ ! -d "$TOP_LEVEL/.git" ]; then
    echo "error: $TOP_LEVEL/.git not found" >&2
    exit 1
fi

DATE="$(date +%Y-%m-%d)"
ARCHIVE="hatchling-platform-$DATE.git.txz"

if [ -d "$HOME/Backups" ]; then
    DEST="$HOME/Backups"
else
    DEST="$HOME"
fi

tar -cJf "$DEST/$ARCHIVE" -C "$TOP_LEVEL/.." hatchling-platform/.git

echo "wrote: $DEST/$ARCHIVE" 
echo "extract with: tar xJf Backups/hatchling-platform-$DATE.git.txz"
echo "then restore files with: git restore ."
