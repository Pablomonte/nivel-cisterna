#!/usr/bin/env python3
"""
Concatena archivos Markdown y genera PDF y/o DOCX via LibreOffice.

Uso básico (desde el directorio con los .md):
    python3 build_doc.py

Pasar archivos explícitamente (en orden):
    python3 build_doc.py propuesta.md anexo-a.md anexo-b.md

Opciones:
    --output   nombre base del archivo de salida  (default: el stem del primer .md)
    --outdir   directorio de salida               (default: mismo que los archivos)
    --formats  pdf docx html — uno o más          (default: pdf docx)
    --title    título del documento HTML
    --no-break omitir saltos de página entre archivos

Requisitos:
    pip install markdown
    libreoffice  (apt install libreoffice / brew install --cask libreoffice)
"""

import argparse
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path


def check_deps():
    try:
        import markdown  # noqa: F401
    except ImportError:
        sys.exit("Falta: pip install markdown")
    if not shutil.which("libreoffice"):
        sys.exit(
            "Falta LibreOffice.  Instalá con:\n"
            "  Linux:  sudo apt install libreoffice\n"
            "  macOS:  brew install --cask libreoffice\n"
            "  Windows: https://www.libreoffice.org/download/"
        )


CSS = """
<style>
  @page { margin: 2.5cm 2.5cm 3cm 2.5cm; }
  body { font-family: 'Liberation Serif', Georgia, serif; font-size: 11pt;
         max-width: 800px; margin: 40px auto; line-height: 1.6; color: #111; }
  h1 { font-size: 16pt; border-bottom: 2px solid #333; padding-bottom: 6px; margin-top: 40px; }
  h2 { font-size: 13pt; color: #222; margin-top: 30px; }
  h3 { font-size: 11pt; font-style: italic; margin-top: 20px; }
  table { border-collapse: collapse; width: 100%; margin: 16px 0; font-size: 10pt; }
  th { background: #e8e8e8; border: 1px solid #999; padding: 6px 10px; text-align: left; }
  td { border: 1px solid #bbb; padding: 5px 10px; vertical-align: top; }
  tr:nth-child(even) td { background: #f7f7f7; }
  hr { border: none; border-top: 1px solid #ccc; margin: 24px 0; }
  code, pre { font-family: 'Liberation Mono', monospace; background: #f4f4f4;
               padding: 2px 5px; border-radius: 3px; font-size: 9.5pt; }
  pre { padding: 10px; overflow-x: auto; }
  .page-break { page-break-after: always; }
</style>
"""


def build_html(files: list[Path], title: str, page_breaks: bool) -> str:
    import markdown

    md = markdown.Markdown(extensions=["tables", "fenced_code", "nl2br", "sane_lists"])
    parts = []
    for i, path in enumerate(files):
        md.reset()
        body = md.convert(path.read_text(encoding="utf-8"))
        if page_breaks and i < len(files) - 1:
            body += '\n<div class="page-break"></div>\n'
        parts.append(body)

    return (
        f'<!DOCTYPE html>\n<html lang="es">\n<head>\n'
        f'  <meta charset="UTF-8">\n  <title>{title}</title>\n'
        f'  {CSS}\n</head>\n<body>\n'
        + "\n".join(parts)
        + "\n</body>\n</html>\n"
    )


def lo_convert(src: Path, fmt: str) -> Path:
    """Convierte src al formato dado; devuelve el archivo resultante."""
    tmp = Path(tempfile.mkdtemp())
    r = subprocess.run(
        ["libreoffice", "--headless", "--convert-to", fmt, "--outdir", str(tmp), str(src)],
        capture_output=True, text=True,
    )
    out = tmp / src.with_suffix(f".{fmt}").name
    if r.returncode != 0 or not out.exists():
        sys.exit(f"LibreOffice falló al convertir a {fmt}:\n{r.stderr or '(sin mensajes)'}")
    return out


def main():
    parser = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter
    )
    parser.add_argument(
        "files", nargs="*", type=Path,
        help="Archivos .md a combinar (en orden). Si se omite, usa todos los .md del directorio.",
    )
    parser.add_argument("--output", "-o", help="Nombre base del archivo de salida (sin extensión)")
    parser.add_argument(
        "--outdir", type=Path, default=None,
        help="Directorio de salida (default: directorio del primer archivo de entrada)",
    )
    parser.add_argument(
        "--formats", nargs="+", default=["pdf", "docx"],
        choices=["pdf", "docx", "html"],
        metavar="FMT",
        help="Formatos a generar: pdf docx html (default: pdf docx)",
    )
    parser.add_argument("--title", default=None, help="Título del documento HTML")
    parser.add_argument(
        "--no-break", dest="page_breaks", action="store_false",
        help="No insertar saltos de página entre archivos",
    )
    args = parser.parse_args()

    check_deps()

    # Resolver archivos de entrada
    if args.files:
        files = [p.resolve() for p in args.files]
        missing = [f for f in files if not f.exists()]
        if missing:
            sys.exit("Archivos no encontrados:\n" + "\n".join(str(m) for m in missing))
    else:
        cwd_mds = sorted(Path.cwd().glob("*.md"))
        if not cwd_mds:
            sys.exit("No se encontraron archivos .md en el directorio actual.")
        files = cwd_mds
        print(f"Usando {len(files)} archivos .md del directorio actual.")

    outdir = (args.outdir or files[0].parent).resolve()
    outdir.mkdir(parents=True, exist_ok=True)

    stem = args.output or files[0].stem
    title = args.title or stem.replace("-", " ").replace("_", " ").title()

    # Generar HTML intermedio
    html_content = build_html(files, title, args.page_breaks)
    html_path = outdir / f"{stem}.html"

    if "html" in args.formats:
        html_path.write_text(html_content, encoding="utf-8")
        print(f"OK  {html_path}")
    else:
        # HTML temporal, sólo como paso intermedio
        tmp_html = Path(tempfile.mkdtemp()) / f"{stem}.html"
        tmp_html.write_text(html_content, encoding="utf-8")
        html_path = tmp_html

    if not html_path.exists():
        html_path.write_text(html_content, encoding="utf-8")

    # PDF
    if "pdf" in args.formats:
        src = lo_convert(html_path, "pdf")
        dest = outdir / f"{stem}.pdf"
        shutil.copy2(src, dest)
        print(f"OK  {dest}")

    # DOCX: HTML → ODT → DOCX  (LO no sobreescribe si el destino ya existe)
    if "docx" in args.formats:
        odt = lo_convert(html_path, "odt")
        docx = lo_convert(odt, "docx")
        dest = outdir / f"{stem}.docx"
        shutil.copy2(docx, dest)
        print(f"OK  {dest}")


if __name__ == "__main__":
    main()
