#!/usr/bin/env python3
"""Regenerate the OpenGL 4.6 coverage numbers in docs/coverage-core.md.

The report answers one question quantitatively: which OpenGL 4.6 commands does
YAGLT's public frontend (`include/glcompat/frontend/gl_api.hpp`) actually expose?

Method
------
1. **Command universe** - the spec text (`OpenGL-4.6-Compatibility.md`) is scanned
   for command prototypes (`void Name(`, `boolean Name(`, `uint Name(`, ...),
   including the braced template families the spec uses to fold variants into one
   prototype (e.g. `void Uniform{1234}{if}v(`). Braced families are *expanded* to
   the concrete command names they stand for, so a spec command matches a frontend
   entry point by exact name instead of by fuzzy normalization.
2. **Implemented surface** - every `gl*` function declared in `gl_api.hpp`.
3. The two sets are intersected. Entry points with no spec counterpart are listed
   explicitly (helpers such as `glFlushState`, singular convenience spellings such
   as `glGenQuery`, and commands the spec text spells differently).

Usage
-----
    tools/coverage_report.py            # print the report
    tools/coverage_report.py --update   # also rewrite the generated doc regions

Only the regions between the `<!-- coverage:*:begin -->` / `:end` markers in
docs/coverage-core.md are rewritten; the qualitative per-chapter table is
hand-maintained.
"""
import argparse
import os
import re
import sys
import textwrap

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SPEC = os.path.join(ROOT, "OpenGL-4.6-Compatibility.md")
API = os.path.join(ROOT, "include", "glcompat", "frontend", "gl_api.hpp")
DOC = os.path.join(ROOT, "docs", "coverage-core.md")

# Return types the spec uses for command prototypes. Pointers (`void *`,
# `const ubyte *`, ...) are handled by the separator below, not here.
RETURN_TYPES = (
    r"const\s+ubyte\s*\*|ubyte\s*\*|const\s+void\s*\*|void\s*\*|void|boolean|"
    r"enum|int|uint|sizei|intptr|sizeiptr|int64|uint64|float|double|sync|handle|byte"
)
# A prototype is `<return> [ *] <Name> [{braces}] [ <lowercase suffix>] (`.
# The lowercase suffix folds spec quirks such as `GetBooleani v` (real GL name
# `GetBooleani_v`) into the command name.
PROTO_RE = re.compile(
    r"\b(?:" + RETURN_TYPES + r")(?:\s+\*?|\*)?"
    r"([A-Z][A-Za-z0-9_]*(?:\{[^}\n]*\}[A-Za-z0-9_]*)*)"
    r"(?:\s+([a-z]+))?\s*\("
)

# Compatibility-only commands removed in the core profile (spec appendix E.2.2),
# recognized by prefix. Used only to report the core-profile subset.
COMPAT_PREFIXES = (
    "Accum", "AlphaFunc", "AreTexturesResident", "ArrayElement", "Begin", "Bitmap",
    "CallList", "CallLists", "ClientActiveTexture", "ClipPlane", "Color", "ColorMaterial",
    "ColorPointer", "ColorSubTable", "ColorTable", "ConvolutionFilter", "ConvolutionParameter",
    "CopyColorSubTable", "CopyColorTable", "CopyConvolutionFilter", "CopyPixels",
    "CopyTexImage1D", "DeleteLists", "DisableClientState", "DrawPixels", "EdgeFlag",
    "EnableClientState", "End", "EvalCoord", "EvalMesh", "EvalPoint", "FeedbackBuffer",
    "Fog", "Frustum", "GenLists", "GetClipPlane", "GetColorTable", "GetConvolutionFilter",
    "GetHistogram", "GetLight", "GetMap", "GetMaterial", "GetMinmax", "GetPixelMap",
    "GetPolygonStipple", "GetSeparableFilter", "GetTexEnv", "GetTexGen", "Histogram",
    "Index", "InitNames", "InterleavedArrays", "IsList", "LightModel", "Light", "LineStipple",
    "ListBase", "LoadIdentity", "LoadMatrix", "LoadName", "LoadTransposeMatrix", "Map1",
    "Map2", "MapGrid", "Material", "MatrixMode", "Minmax", "MultMatrix", "MultTransposeMatrix",
    "MultiTexCoord", "NewList", "Normal", "NormalPointer", "Ortho", "PassThrough",
    "PixelMap", "PixelStoref", "PixelTransfer", "PixelZoom", "PolygonStipple", "PopAttrib",
    "PopClientAttrib", "PopMatrix", "PopName", "PrioritizeTextures", "PushAttrib",
    "PushClientAttrib", "PushMatrix", "PushName", "Rasterpos", "RasterPos", "Rect",
    "RenderMode", "ResetHistogram", "ResetMinmax", "Rotate", "Scale", "SecondaryColor",
    "SelectBuffer", "SeparableFilter2D", "ShadeModel", "TexCoord",     "TexEnv", "TexGen",
    "Translate", "Vertex", "EndList", "ClearAccum", "ClearIndex", "IndexMask",
    "ClientAttribDefault", "PushDebugGroup", "PopDebugGroup",
    # GL_ARB_imaging / compatibility-only getters & window-position commands
    # (no core-profile equivalent) recognized by prefix so they are excluded
    # from the core-profile coverage subset.
    "WindowPos", "GetColorTableParameter", "GetConvolutionParameter",
    "GetHistogramParameter", "GetMinmaxParameter", "GetSeparableFilterParameter",
    "GetPixelMap", "GetTexEnv", "GetTexGen", "GetMaterial", "GetLight",
    "GetClipPlane", "GetMap", "GetPolygonStipple",
)


def expand_braces(name):
    """Expand a spec template family into the concrete command names it folds.

    `Uniform{1234}{if}v` -> Uniform1i, Uniform1f, Uniform2i, ... Inside a brace
    group, space-separated chunks are alternatives; a chunk of digits or of
    single-letter type suffixes contributes one option per character, while a
    multi-character chunk starting with `u` (ui, us, ub, ui64) is one option.

    A brace group written with commas lists whole alternatives verbatim instead
    (`UniformMatrix{2x3,3x2,2x4,4x2,3x4,4x3}{fd}v` -> UniformMatrix2x3f,
    UniformMatrix2x3d, ...); without this the per-character rule above would
    shred `2x3` into `2`, `x`, `3`.
    """
    m = re.search(r"\{([^}]*)\}", name)
    if m is None:
        return [name]
    group = m.group(1)
    options = []
    if "," in group:
        options = [chunk.strip() for chunk in group.split(",") if chunk.strip()]
    else:
        for chunk in group.split():
            if len(chunk) > 1 and chunk[0] != "u":
                options.extend(list(chunk))
            else:
                options.append(chunk)
    out = []
    for opt in options:
        out.extend(expand_braces(name[: m.start()] + opt + name[m.end():]))
    return out


def spec_commands(text):
    names = set()
    for m in PROTO_RE.finditer(text):
        name = m.group(1)
        if m.group(2):  # spec writes the vector suffix as a separate token
            name += "_" + m.group(2)
        for expanded in expand_braces(name):
            names.add(expanded)
    return names


def api_entry_points(text):
    # Declarations only (the header has no definitions); ignore comments.
    stripped = re.sub(r"//[^\n]*", "", text)
    return set(re.findall(r"\b(gl[A-Za-z0-9_]+)\s*\(", stripped))


def is_compat_only(name):
    return any(name.startswith(p) for p in COMPAT_PREFIXES)


# Entry points that are real OpenGL (core profile / GL 4.5 ARB) functions but
# are *not* enumerated as prototypes in `OpenGL-4.6-Compatibility.md`'s command
# index. They are valid and implemented; the spec reference text simply omits
# their DSA-robust prototypes, so the automatic matcher cannot see them. Listing
# them here keeps the "unmatched" list honest instead of implying a defect.
KNOWN_VALID = {
    "glGetnTextureImage",            # GL 4.5 DSA robust (spec only lists GetnTexImage)
    "glGetnCompressedTextureImage",  # GL 4.5 DSA robust (spec only lists GetnCompressedTexImage)
    "glGetnBufferParameteriv",       # GL 4.5 ARB_robustness (spec omits robust prototype)
    "glGetnBufferParameteri64v",     # GL 4.5 ARB_robustness (spec omits robust prototype)
}


def build_report():
    spec = spec_commands(open(SPEC).read())
    api = api_entry_points(open(API).read())
    universe = sorted(spec)
    core = [n for n in universe if not is_compat_only(n)]
    implemented = sorted(api)
    matched = sorted(n for n in implemented if n[2:] in spec)
    unmatched = sorted(n for n in implemented
                       if n[2:] not in spec and n not in KNOWN_VALID)
    extra_valid = sorted(n for n in implemented
                         if n[2:] not in spec and n in KNOWN_VALID)
    core_matched = sorted(n for n in matched if not is_compat_only(n[2:]))
    return {
        "universe": len(universe),
        "core": len(core),
        "entry_points": len(implemented),
        "matched": len(matched),
        "core_matched": len(core_matched),
        "implemented": implemented,
        "unmatched": unmatched,
        "extra_valid": extra_valid,
    }


def format_entry_list(names, width=96):
    return "\n".join(textwrap.wrap(", ".join(names), width=width))


def render_headline(r):
    pct = 100.0 * r["matched"] / r["universe"]
    core_pct = 100.0 * r["core_matched"] / r["core"]
    return (
        "| Universe | Commands | With frontend entry point | Coverage |\n"
        "|----------|---------:|--------------------------:|---------:|\n"
        "| Full spec (compat + core) | {u} | {m} | **~{p:.1f}%** |\n"
        "| Core profile only (compat-only commands removed) | {c} | {cm} | **~{cp:.1f}%** |\n"
        "\n"
        "Generated by `tools/coverage_report.py` from `OpenGL-4.6-Compatibility.md`\n"
        "and `include/glcompat/frontend/gl_api.hpp`. Spec template families (e.g.\n"
        "`void Uniform{{1234}}{{if}}(`) are expanded to the concrete commands they stand\n"
        "for, so a match means an exact command name is exposed by the frontend.\n"
    ).format(u=r["universe"], m=r["matched"], c=r["core"], cm=r["core_matched"],
             p=pct, cp=core_pct)


def render_surface(r):
    lines = []
    lines.append("%d `gl*` entry points; %d map to a spec command."
                 % (r["entry_points"], r["matched"]))
    if r["unmatched"]:
        lines.append("")
        lines.append("Genuinely extra (helpers / non-spec convenience spellings): "
                     + ", ".join("`%s`" % n for n in r["unmatched"]) + ".")
    if r["extra_valid"]:
        lines.append("")
        lines.append("Valid GL but absent from this spec's prototype index: "
                     + ", ".join("`%s`" % n for n in r["extra_valid"]) + ".")
    lines.append("")
    lines.append("All implemented entry points, listed alphabetically:")
    lines.append("")
    lines.append(format_entry_list(r["implemented"]))
    return "\n".join(lines) + "\n"


def replace_region(text, key, body):
    begin = "<!-- coverage:%s:begin -->" % key
    end = "<!-- coverage:%s:end -->" % key
    if begin not in text or end not in text:
        raise SystemExit("marker %s missing in %s" % (key, DOC))
    head = text[: text.index(begin) + len(begin)]
    tail = text[text.index(end):]
    return head + "\n" + body.rstrip() + "\n" + tail


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--update", action="store_true",
                    help="rewrite the generated regions of docs/coverage-core.md")
    args = ap.parse_args()

    r = build_report()
    sys.stdout.write(render_headline(r))
    sys.stdout.write("\n%d entry points without a spec command: %s\n"
                     % (len(r["unmatched"]), ", ".join(r["unmatched"])))
    if r["extra_valid"]:
        sys.stdout.write("%d valid GL entry points absent from spec index: %s\n"
                         % (len(r["extra_valid"]), ", ".join(r["extra_valid"])))

    if args.update:
        doc = open(DOC).read()
        doc = replace_region(doc, "headline", render_headline(r))
        doc = replace_region(doc, "surface", render_surface(r))
        open(DOC, "w").write(doc)
        sys.stderr.write("coverage_report: updated %s\n" % DOC)
    return 0


if __name__ == "__main__":
    sys.exit(main())
