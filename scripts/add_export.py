#!/usr/bin/env python3
"""
Mechanically add ED25519_EXPORT to all extern function declarations in headers.

Extern declarations are identified as lines starting with a return type
(void, int, unsigned char, uint32_t, const ed25519_dispatch_table)
followed by a function name and '(', that are NOT preceded by
static/inline/template keywords.

Files that don't transitively include ed25519_export.h get a direct include added.
"""

import os
import re

INCLUDE_DIR = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "include")

# Return types that start extern function declarations
RETURN_TYPE_PATTERN = re.compile(
    r'^(void|int|unsigned char|uint32_t|const ed25519_dispatch_table)\s'
)

# Files that include ed25519_platform.h transitively (and thus ed25519_export.h)
# ed25519_platform.h -> ed25519_export.h
# fe.h -> ed25519_platform.h
# ge.h -> fe.h -> ed25519_platform.h
# ed25519_cpuid.h -> ed25519_platform.h
# ed25519_dispatch.h -> ed25519_platform.h
TRANSITIVE_INCLUDES = {
    'ed25519_platform.h', 'ed25519_export.h',
    'fe.h', 'ge.h',
    'ed25519_cpuid.h', 'ed25519_dispatch.h',
    # Headers that include ge.h or fe.h
    'fe_add.h', 'fe_sub.h', 'fe_mul.h', 'fe_sq.h',
}


def has_transitive_export(lines):
    """Check if the file already includes ed25519_export.h transitively."""
    for line in lines:
        stripped = line.strip()
        if stripped.startswith('#include'):
            for inc in TRANSITIVE_INCLUDES:
                if inc in stripped:
                    return True
    return False


def is_extern_decl(lines, i):
    """Check if line i is an extern function declaration (not static inline or template)."""
    line = lines[i]
    stripped = line.lstrip()

    # Already has ED25519_EXPORT
    if 'ED25519_EXPORT' in stripped:
        return False

    # Must match a return type pattern
    if not RETURN_TYPE_PATTERN.match(stripped):
        return False

    # Must contain '(' (function declaration)
    # Check this line and next few lines for multiline declarations
    has_paren = '(' in stripped
    if not has_paren:
        return False

    # Must NOT be preceded by static/inline/template on current or previous line
    if 'static ' in stripped or 'inline ' in stripped:
        return False

    if i > 0:
        prev = lines[i - 1].strip()
        if prev.startswith('static') or prev.startswith('inline') or prev.startswith('template'):
            return False

    # Check it's a declaration not a definition (look for ; before {)
    # Scan forward to find either ; or { to determine
    combined = stripped
    j = i + 1
    while j < len(lines) and ';' not in combined and '{' not in combined:
        combined += lines[j]
        j += 1

    # If we find '{' before ';', it's a definition, not a declaration
    semi_pos = combined.find(';')
    brace_pos = combined.find('{')
    if brace_pos != -1 and (semi_pos == -1 or brace_pos < semi_pos):
        return False

    return True


def add_export_include(lines):
    """Add #include "ed25519_export.h" after the header guard #define."""
    for i, line in enumerate(lines):
        stripped = line.strip()
        if stripped.startswith('#define ') and ('_H' in stripped or '_h' in stripped):
            # Check it's a header guard (follows #ifndef)
            if i > 0 and lines[i - 1].strip().startswith('#ifndef'):
                lines.insert(i + 1, '\n#include "ed25519_export.h"\n')
                return lines
    return lines


def process_file(filepath):
    """Process a single header file."""
    with open(filepath, 'r', encoding='utf-8') as f:
        lines = f.readlines()

    modified = False
    modified_lines = []
    i = 0
    while i < len(lines):
        if is_extern_decl(lines, i):
            # Preserve leading whitespace
            leading = len(lines[i]) - len(lines[i].lstrip())
            ws = lines[i][:leading]
            modified_lines.append(ws + 'ED25519_EXPORT ' + lines[i].lstrip())
            modified = True
        else:
            modified_lines.append(lines[i])
        i += 1

    if modified:
        # Check if file needs a direct include
        if not has_transitive_export(modified_lines):
            modified_lines = add_export_include(modified_lines)

        with open(filepath, 'w', encoding='utf-8', newline='\n') as f:
            f.writelines(modified_lines)

        return True
    return False


def main():
    total_modified = 0
    total_exports = 0

    for root, dirs, files in os.walk(INCLUDE_DIR):
        # Skip SIMD internal headers (avx2/, ifma/) - those contain only
        # static inline or macro code, no extern declarations
        rel = os.path.relpath(root, INCLUDE_DIR)
        if 'avx2' in rel or 'ifma' in rel:
            continue

        for f in sorted(files):
            if not f.endswith('.h'):
                continue

            filepath = os.path.join(root, f)

            # Count exports before
            with open(filepath, 'r', encoding='utf-8') as fh:
                before = fh.read().count('ED25519_EXPORT')

            if process_file(filepath):
                with open(filepath, 'r', encoding='utf-8') as fh:
                    after = fh.read().count('ED25519_EXPORT')
                exports_added = after - before
                total_exports += exports_added
                total_modified += 1
                print(f"  {os.path.relpath(filepath, INCLUDE_DIR)}: +{exports_added} exports")

    print(f"\nTotal: {total_modified} files modified, {total_exports} ED25519_EXPORT annotations added")


if __name__ == '__main__':
    main()
