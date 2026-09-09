#!/usr/bin/env python3
"""Fabrique les deux PDF de docs/pdf.

Deux choses, dans cet ordre :

  1. Les annexes B et C du manuel sont REGENEREES depuis `scratchvj layout` et
     `scratchvj effects`, entre les marqueurs `<!--GEN:...-->` du HTML. Le
     catalogue imprime ne peut donc pas diverger du code -- meme raison que
     core/effect.h, qui tient la prose et l'implementation dans le meme fichier.
     Corollaire : recompiler AVANT de lancer ce script, sinon les annexes
     decrivent un binaire perime.

  2. Les deux HTML sont imprimes par Chrome en mode headless. C'est la seule
     chaine PDF disponible sur cette machine (ni pandoc, ni weasyprint, ni
     reportlab). `--allow-file-access-from-files` n'est pas optionnel : sans lui
     la feuille de style et les @font-face en file:// sont ignores EN SILENCE, et
     le PDF sort en Times sans qu'aucune erreur ne soit emise.
"""

import html
import io
import os
import re
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))

CHROME_CANDIDATES = [
    os.environ.get("CHROME", ""),
    r"C:\Program Files\Google\Chrome\Application\chrome.exe",
    r"C:\Program Files (x86)\Google\Chrome\Application\chrome.exe",
    r"C:\Program Files (x86)\Microsoft\Edge\Application\msedge.exe",
    "/Applications/Google Chrome.app/Contents/MacOS/Google Chrome",
    "google-chrome",
    "chromium",
]

SCRATCHVJ_CANDIDATES = [
    os.path.join(ROOT, "build", "scratchvj", "Release", "scratchvj.exe"),
    os.path.join(ROOT, "build", "scratchvj", "RelWithDebInfo", "scratchvj.exe"),
    os.path.join(ROOT, "build-ui", "scratchvj", "Release", "scratchvj.exe"),
    os.path.join(ROOT, "build", "scratchvj", "scratchvj"),
]


def find(candidates, what):
    for c in candidates:
        if c and (os.path.exists(c) or os.path.sep not in c):
            return c
    sys.exit("introuvable : %s (essaye : %s)" % (what, ", ".join(filter(None, candidates))))


def run(exe, *args):
    out = subprocess.run([exe, *args], capture_output=True, check=True).stdout
    return out.decode("utf-8", "replace")


def layout_table(text):
    """Annexe B : les controles, en deux colonnes."""
    rows = []
    for line in text.splitlines():
        if not line.startswith("  "):
            continue
        parts = line.split()
        if len(parts) == 2:
            rows.append(parts)
    if not rows:
        sys.exit("`scratchvj layout` n'a rien donne d'exploitable")
    out = ['<div class="cols2" style="font-size:8.6pt;line-height:1.75">']
    for cid, kind in rows:
        out.append(
            '<div style="break-inside:avoid;border-bottom:1px solid #eae5dd">'
            '<span class="mono">%s</span>'
            '<span style="float:right;color:#6c6862;font-size:7.6pt">%s</span></div>'
            % (html.escape(cid), html.escape(kind))
        )
    out.append("</div>")
    return "\n".join(out), len(rows)


def effects_table(text):
    """Annexe C : les paires, puis les effets video seuls."""
    pairs, solo, mode, cur = [], [], None, None
    for line in text.splitlines():
        s = line.strip()
        if s == "PAIRES":
            mode = "p"
            continue
        if s.startswith("VID") and "SEULS" in s:
            mode = "s"
            continue
        if mode == "p":
            if s.startswith("audio "):
                cur[2] = s[6:].strip()
            elif s.startswith("vid") and len(s) > 6 and s[5] in " \u00e9":
                cur[3] = s.split(None, 1)[1].strip()
                pairs.append(cur)
                cur = None
            else:
                m = re.match(r"(\S+)\s+\[(\w+)\]", s)
                if m:
                    cur = [m.group(1), m.group(2), "", ""]
        elif mode == "s" and s:
            bits = s.split(None, 1)
            if len(bits) == 2:
                solo.append(bits)
    if not pairs or not solo:
        sys.exit("`scratchvj effects` n'a rien donne d'exploitable")

    out = ['<p class="eyebrow" style="margin-top:0">Seize paires audio / vid\u00e9o</p>',
           '<table class="long"><tr><th style="width:15%">Effet</th>'
           '<th style="width:12%">Relation</th><th style="width:36%">Audio</th>'
           '<th>Vid\u00e9o</th></tr>']
    for eid, rel, a, v in pairs:
        out.append(
            '<tr><td class="mono">%s</td><td><span class="tag %s">%s</span></td>'
            '<td>%s</td><td>%s</td></tr>'
            % (html.escape(eid), "ok" if rel == "identique" else "part",
               html.escape(rel), html.escape(a), html.escape(v))
        )
    out.append("</table>")
    out.append('<p class="eyebrow">Seize effets vid\u00e9o sans \u00e9quivalent sonore</p>')
    out.append('<table class="long"><tr><th style="width:15%">Effet</th>'
               "<th>Ce qu'il fait</th></tr>")
    for eid, desc in solo:
        out.append('<tr><td class="mono">%s</td><td>%s</td></tr>'
                   % (html.escape(eid), html.escape(desc)))
    out.append("</table>")
    return "\n".join(out), (len(pairs), len(solo))


def splice(doc, marker, block):
    pattern = re.compile(r"(<!--GEN:%s-->).*?(<!--/GEN:%s-->)" % (marker, marker), re.S)
    if not pattern.search(doc):
        sys.exit("marqueur GEN:%s absent du manuel" % marker)
    return pattern.sub(lambda m: "%s\n%s\n%s" % (m.group(1), block, m.group(2)), doc)


def print_pdf(chrome, source, target):
    subprocess.run(
        [chrome, "--headless", "--disable-gpu", "--no-pdf-header-footer",
         # sans ce drapeau, _style.css et les polices sont ignores en silence
         "--allow-file-access-from-files",
         "--print-to-pdf=" + target,
         "file:///" + source.replace("\\", "/")],
        check=True, capture_output=True,
    )
    return os.path.getsize(target)


def main():
    chrome = find(CHROME_CANDIDATES, "Chrome ou Edge")
    svj = find(SCRATCHVJ_CANDIDATES, "le binaire scratchvj (compiler d'abord)")

    manual = os.path.join(HERE, "manuel.html")
    doc = io.open(manual, encoding="utf-8").read()

    controls, n_controls = layout_table(run(svj, "layout"))
    effects, n_effects = effects_table(run(svj, "effects"))
    doc = splice(doc, "LAYOUT", controls)
    doc = splice(doc, "EFFECTS", effects)
    io.open(manual, "w", encoding="utf-8").write(doc)
    print("annexes regenerees : %d controles, %d paires + %d effets video"
          % (n_controls, n_effects[0], n_effects[1]))

    for src, dst in (("manuel.html", "scratchvj-manuel.pdf"),
                     ("argumentaire.html", "scratchvj-argumentaire.pdf")):
        size = print_pdf(chrome, os.path.join(HERE, src), os.path.join(HERE, dst))
        print("%-32s %6d Ko" % (dst, round(size / 1024)))


if __name__ == "__main__":
    main()
