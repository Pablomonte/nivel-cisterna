#!/usr/bin/env python3
"""Convierte el HTML generado por LibreOffice a Markdown limpio."""

import re
import sys
from html.parser import HTMLParser
from pathlib import Path


class LO2MD(HTMLParser):
    def __init__(self):
        super().__init__()
        self.out = []
        self._stack = []      # tag stack for context checks
        self._buf = ""        # inline text buffer (accumulates until block end)
        self._table = []      # list of (row_cells, is_header) tuples
        self._row = []        # current row cells
        self._cell_buf = ""   # inline buffer inside a table cell
        self._in_cell = False
        self._in_pre = False
        self._skip = False    # True while inside <head>
        self._ol_counter = [] # stack for ordered list counters

    # ── helpers ──────────────────────────────────────────────────────────

    def _emit(self, text):
        self.out.append(text)

    def _in(self, *tags):
        return any(t in self._stack for t in tags)

    def _append(self, text):
        """Append text to whatever buffer is active."""
        if self._in_cell:
            self._cell_buf += text
        else:
            self._buf += text

    # ── tag open ─────────────────────────────────────────────────────────

    def handle_starttag(self, tag, attrs):
        self._stack.append(tag)
        if tag == "head":
            self._skip = True
        if self._skip:
            return

        if tag in ("h1", "h2", "h3"):
            self._buf = ""
        elif tag == "ul":
            self._emit("\n")
        elif tag == "ol":
            self._ol_counter.append(0)
            self._emit("\n")
        elif tag == "li":
            self._buf = ""
        elif tag == "table":
            self._table = []
        elif tag == "tr":
            self._row = []
        elif tag in ("td", "th"):
            self._cell_buf = ""
            self._buf = ""
            self._in_cell = True
        elif tag == "hr":
            self._emit("\n---\n\n")
        elif tag == "pre":
            self._buf = ""
            self._in_pre = True
        elif tag == "strong":
            self._append("**")
        elif tag == "em":
            self._append("*")
        elif tag in ("code",) and not self._in_pre:
            self._append("`")
        elif tag == "br":
            if self._in_pre:
                self._append("\n")
            else:
                self._append(" ")

    # ── tag close ────────────────────────────────────────────────────────

    def handle_endtag(self, tag):
        if tag == "head":
            self._skip = False
        # pop stack
        if tag in self._stack:
            idx = len(self._stack) - 1 - self._stack[::-1].index(tag)
            self._stack = self._stack[:idx] + self._stack[idx+1:]
        if self._skip:
            return

        if tag == "strong":
            self._append("**")
        elif tag == "em":
            self._append("*")
        elif tag == "code" and not self._in_pre:
            self._append("`")

        elif tag == "h1":
            self._emit(f"\n# {self._buf.strip()}\n\n")
            self._buf = ""
        elif tag == "h2":
            self._emit(f"\n## {self._buf.strip()}\n\n")
            self._buf = ""
        elif tag == "h3":
            self._emit(f"\n### {self._buf.strip()}\n\n")
            self._buf = ""

        elif tag == "p":
            text = self._buf.strip()
            self._buf = ""
            if self._in_cell:
                # p inside cell: append to cell buf with a space separator
                if self._cell_buf and text:
                    self._cell_buf += " " + text
                elif text:
                    self._cell_buf = text
            elif self._in("li"):
                # p inside li: don't emit yet, keep for </li>
                self._buf = text
            else:
                if text:
                    self._emit(f"{text}\n\n")

        elif tag == "li":
            text = self._buf.strip()
            self._buf = ""
            if self._in("ol"):
                if self._ol_counter:
                    self._ol_counter[-1] += 1
                    n = self._ol_counter[-1]
                else:
                    n = 1
                self._emit(f"{n}. {text}\n")
            else:
                self._emit(f"- {text}\n")

        elif tag == "ul":
            self._emit("\n")
        elif tag == "ol":
            if self._ol_counter:
                self._ol_counter.pop()
            self._emit("\n")

        elif tag in ("td", "th"):
            self._in_cell = False
            # merge cell buf + any dangling inline buf
            cell = (self._cell_buf + " " + self._buf).strip()
            self._buf = ""
            self._cell_buf = ""
            self._row.append(cell)

        elif tag == "tr":
            if self._row:
                attrs_dict = {}
                is_th = any(
                    tag == "th"
                    for tag in ["th"]
                    if tag in ["th"]
                )
                # detect header by checking if row came from <th> cells
                # (we track it via the tag name we saw — use first cell's source)
                self._table.append(self._row[:])
                self._row = []

        elif tag == "table":
            self._render_table()
            self._table = []

        elif tag == "pre":
            self._in_pre = False
            text = self._buf.strip()
            self._buf = ""
            if text:
                self._emit(f"\n```\n{text}\n```\n\n")

    # ── text ─────────────────────────────────────────────────────────────

    def handle_data(self, data):
        if self._skip:
            return
        if self._in_pre:
            self._append(data)
        else:
            # collapse whitespace (LO wraps long lines with tabs/newlines)
            data = re.sub(r"[\t\n\r]+", " ", data)
            self._append(data)

    def handle_entityref(self, name):
        entities = {"amp": "&", "lt": "<", "gt": ">", "nbsp": " ",
                    "quot": '"', "apos": "'"}
        self._append(entities.get(name, f"&{name};"))

    def handle_charref(self, name):
        ch = chr(int(name[1:], 16) if name.startswith("x") else int(name))
        self._append(ch)

    # ── table rendering ──────────────────────────────────────────────────

    def _render_table(self):
        if not self._table:
            return
        ncols = max(len(r) for r in self._table)
        lines = ["\n"]
        for i, row in enumerate(self._table):
            while len(row) < ncols:
                row.append("")
            row = [re.sub(r"\s+", " ", c).strip() for c in row]
            lines.append("| " + " | ".join(row) + " |")
            if i == 0:
                lines.append("|" + "|".join(["---"] * ncols) + "|")
        lines.append("\n")
        self._emit("\n".join(lines) + "\n")

    # ── output ───────────────────────────────────────────────────────────

    def result(self):
        text = "".join(self.out)
        text = re.sub(r"\n{4,}", "\n\n\n", text)
        text = "\n".join(line.rstrip() for line in text.splitlines())
        return text.strip() + "\n"


def main():
    src = Path(sys.argv[1]) if len(sys.argv) > 1 else Path("/tmp/propuesta-coope-san-jorge-completa.html")
    dst = Path(sys.argv[2]) if len(sys.argv) > 2 else src.with_suffix(".md")

    parser = LO2MD()
    parser.feed(src.read_text(encoding="utf-8"))
    md = parser.result()
    dst.write_text(md, encoding="utf-8")
    print(f"OK  {dst}  ({len(md.splitlines())} líneas)")


if __name__ == "__main__":
    main()
