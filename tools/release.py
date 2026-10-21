#!/usr/bin/env python3
"""Release notes and file labels for a GitHub release.

GitHub lists a release's files from A to Z and there is no way to reorder
them, so the notes carry the list in a useful order (NumPlay first, then its
discreet versions, then each game on its own in the launcher's order) with a
link to each file, and every file gets a label saying what it is.

- --notes FILE: writes the notes, .github/release.md (the introduction) and
  then the list;
- prints one `path#label` per line, the arguments of `gh release create`.

Usage: release.py --tag v1.3.0 --notes build/release-notes.md
"""
import argparse
import json
import os
import re

ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..")
REPO = "https://github.com/Mason363/NumPlay"
# in games.json, but not games
EXTRAS = set()
# games too big to fit next to NumPlay on the calculator: only on their own, in a section of their own
ALONE = {
    "supermariobros": ("Super Mario Bros.", "the complete original 32-stage campaign, adapted by Bobydeluxe. "
                       "A standalone big game: install it separately from the NumPlay launcher"),
    "celeste": ("Celeste", "the whole climb up Celeste Mountain"),
    "hollowknight": ("Hollow Knight", "from King's Pass to Hornet in Greenpath"),
    "undertale": ("Undertale", "the whole Underground, to the barrier and the neutral ending"),
    "subway": ("Subway Surfers", "the endless run down the subway's tracks, in 3D"),
    "fnaf": ("Five Nights at Freddy's", "five nights at Freddy Fazbear's Pizza, the 6th and the Custom Night"),
    "hillclimb": ("Hill Climb Racing", "Newton Bill's ride up the hills: ten stages, eight vehicles"),
    "championisland": ("Champion Island", "the Doodle Champion Island Games"),
    "numpowder": ("NumPowder", "The Powder Toy on the calculator, with nearly 200 elements"),
    "finalearth2": ("The Final Earth 2", "build a city on floating islands in space"),
    "doom": ("Doom", "Knee-Deep in the Dead, all nine levels"),
    "isaac": ("The Binding of Isaac", "a whole run, from the Basement to Sheol and the Cathedral"),
    "pvz": ("Plants vs. Zombies", "the whole Adventure, from the front lawn to Dr. Zomboss"),
    "leveldevil": ("Level Devil", "Unept's troll platformer, all sixteen worlds"),
    "sonic": ("Sonic the Hedgehog", "the whole 1991 game, Green Hill to the Final Zone"),
    "numvisuals": ("NumVisuals", "moving backgrounds with a clock, a stopwatch, a timer, a counter or your own words"),
    "cookieclicker": ("Cookie Clicker", "the whole game, to the grandmapocalypse, ascension and the dragon"),
    "jetpackjoyride": ("Jetpack Joyride", "Barry's run through the lab, with five vehicles and the Final Spin"),
}
VARIANTS = [
    ("NumPlay-Invisible.nwa", "The same app, hidden: a blank icon with no name"),
    ("NumPlay-Matrices.nwa", "The same app, disguised as a math app called Matrices"),
]
# other languages, and Upsilon (see the README)
LANGUAGES = [
    ("NumPlay-French.nwa", "NumPlay en français : tous les jeux, en français"),
    ("NumPlay-Chinese.nwa", "NumPlay 中文版：所有游戏，简体中文"),
]
UPSILON = ("NumPlay-Upsilon.nwa", "For calculators running Upsilon (N0110, N0115): the 22 games that fit in the memory "
           "Upsilon gives apps. Install it with nwlink, see the README")


def standalone_names():
    """Game id -> its own .nwa, from the Makefile (APP_<id> = path:Name.nwa)."""
    names = {}
    for line in open(os.path.join(ROOT, "Makefile")):
        m = re.match(r"APP_(\w+)\s*=\s*\S+:(\S+\.nwa)\s*$", line)
        if m:
            names[m.group(1)] = m.group(2)
    return names


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--tag", required=True)
    ap.add_argument("--notes", required=True)
    ap.add_argument("--build", default="build")
    a = ap.parse_args()
    games = json.load(open(os.path.join(ROOT, "games", "games.json")))
    apps = standalone_names()
    count = sum(1 for g in games if g["id"] not in EXTRAS)
    extras = " and ".join(g["title"] for g in games if g["id"] in EXTRAS)
    everything = f"All {count} games" + (f" and {extras}" if extras else "") + " in one app"

    def link(name):
        return f"[{name}]({REPO}/releases/download/{a.tag}/{name})"

    files = [(os.path.join(a.build, "NumPlay.nwa"), f"NumPlay.nwa · Start here: {everything[0].lower() + everything[1:]}")]
    notes = ["### Start here", "", "| File | |", "| --- | --- |",
             f"| **{link('NumPlay.nwa')}** | **{everything}.** Start here! |", "",
             "### The same app, in disguise", "", "| File | |", "| --- | --- |"]
    for name, what in VARIANTS:
        files.append((os.path.join(a.build, name), f"{name} · {what}"))
        notes.append(f"| {link(name)} | {what} |")
    notes += ["", "### In French and Chinese", "", "| File | |", "| --- | --- |"]
    for name, what in LANGUAGES:
        files.append((os.path.join(a.build, name), f"{name} · {what}"))
        notes.append(f"| {link(name)} | {what} |")
    # the games too big to share the calculator with NumPlay, each its own app
    alone = [gid for gid in ALONE if gid in apps]
    if alone:
        notes += ["", "### Individual Games", "", "| File | |", "| --- | --- |"]
        for gid in alone:
            title, what = ALONE[gid]
            name, what = apps[gid], f"{title}: {what}"
            files.append((os.path.join(a.build, "apps", name), f"{name} · {what}"))
            notes.append(f"| {link(name)} | {what} |")
    name, what = UPSILON
    notes += ["", "### On Upsilon", "", "| File | |", "| --- | --- |", f"| {link(name)} | {what} |"]
    files.append((os.path.join(a.build, name), f"{name} · {what}"))
    notes += ["", "### Each one on its own", "", "In the launcher's order.", "", "| File | |", "| --- | --- |"]

    for g in games:
        name = g.get("app") or apps.get(g["id"])
        if not name:
            continue
        tagline = g["tagline"][0].lower() + g["tagline"][1:]
        what = f"Only {g['title']}: {tagline}"
        files.append((os.path.join(a.build, "apps", name), f"{name} · {what}"))
        notes.append(f"| {link(name)} | {what} |")
    notes += ["", "<sub>The file list below is sorted A to Z by GitHub. Each file's label says what it is.</sub>"]

    intro = open(os.path.join(ROOT, ".github", "release.md")).read().rstrip()
    with open(a.notes, "w") as f:
        f.write(intro + "\n\n" + "\n".join(notes) + "\n")
    for path, label in files:
        if not os.path.exists(path):
            raise SystemExit(f"release: missing {path}")
        if len(label) > 255:  # (GitHub refuses longer labels, and the release with them)
            raise SystemExit(f"release: {os.path.basename(path)}'s label is {len(label)} characters, 255 at most")
        print(f"{path}#{label}")


if __name__ == "__main__":
    main()
