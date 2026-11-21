# Third-party components and credits

ArgMap uses the following third-party libraries. The full, unmodified FLTK
1.3.11 source distribution is included under `third_party/fltk-1.3.11/` so the
project can be built without separately installing FLTK. FLTK itself bundles
source for zlib, libpng and libjpeg; those bundled copies are redistributed
exactly as they appear in the official FLTK release.

## FLTK (Fast Light Toolkit) 1.3.11

- Used for: GUI widgets, event loop, drawing primitives, file/color dialogs and
  the FLUID UI designer (`.fl` → C++ generation).
- Upstream: <https://www.fltk.org/> — source release:
  <https://github.com/fltk/fltk/releases/download/release-1.3.11/fltk-1.3.11-source.tar.gz>
- SHA-256: `92805abc84505e3e7e27aec775ab0754ecb4182fe2d8ff2a9d568ccdcb2811ac`
  (see `third_party/fltk-1.3.11-SHA256SUMS.txt`).
- License: GNU Library General Public License version 2 (the original LGPL,
  June 1991) **with FLTK exceptions** that explicitly permit static linking
  without requiring the application source. Full text:
  `third_party/fltk-1.3.11/COPYING`.
- Upstream credits: `third_party/fltk-1.3.11/CREDITS`.

Core developers listed by upstream: Greg Ercolano, Manolo Gouy, Ian MacArthur,
Matthias Melcher, Albrecht Schlosser. Previous developers: Fabien Costantini,
Craig P. Earls, Curtis Edwards, Gustavo Hime, Talbot Hughes, Lauri Kasanen,
Robert Kesterson, Pierre Ossman, James Dean Palmer, Vincent Penne, Bill Spitzak,
Michael Sweet, Carl Thompson, Nafees Bin Zafar. The upstream CREDITS file also
lists many additional contributors; it is included unchanged.

> FLTK's own source files carry the copyright notices of Bill Spitzak and the
> other FLTK authors. We did not modify the vendored FLTK tree.

## zlib 1.3.1 (bundled inside FLTK)

- Used for: raw PNG compression in `data/PngExporter.cpp`.
- FLTK 1.3.11 bundles zlib 1.3.1 in `third_party/fltk-1.3.11/zlib/`; its public
  headers remap symbols with an `fltk_z_` prefix to avoid collisions. The CMake
  build uses that bundled copy (`fltk_z` target), and its `zlib.h` is added to
  the include path so ArgMap's `compress2`/`compressBound` calls bind correctly.
- Authors: Jean-loup Gailly and Mark Adler.
- License: zlib license (permissive, attribution appreciated). The official
  1.3.1 license text is copied to
  `third_party/licenses/zlib-1.3.1-LICENSE.txt`; the exact notice shipped by
  FLTK is also in `third_party/fltk-1.3.11/zlib/README`.
- Modification notice: FLTK applies `fltk_`-prefixed external symbols to its
  bundled zlib (documented in `third_party/fltk-1.3.11/README.bundled-libs.txt`
  and visible in `zlib.h`/`zconf.h`). ArgMap did not modify it further. The
  bundled source keeps upstream copyright/license comments.

## libpng (bundled inside FLTK)

- Used indirectly by FLTK for PNG image reading. ArgMap's own PNG export uses
  zlib directly and does not call libpng.
- Source: `third_party/fltk-1.3.11/png/`.
- Authors: The PNG Reference Library Authors; Cosmin Truta; Glenn Randers-Pehrson;
  Andreas Dilger; Guy Eric Schalnat and Group 42, Inc.
- License: PNG Reference Library License (see `third_party/licenses/libpng-LICENSE.txt`).

## Independent JPEG Group (libjpeg, bundled inside FLTK)

- Used indirectly by FLTK for JPEG image reading.
- Source: `third_party/fltk-1.3.11/jpeg/`.
- Authors: Tom Lane, Guido Vollbeding, Philip Gladstone, Bill Allombert,
  Jim Boucher, Lee Crocker, Bob Friesenhahn, Ben Jackson, John Korejwa,
  Julian Minguillon, Luis Ortiz, George Phillips, Davide Rossi, Ge' Weijers
  and other members of the Independent JPEG Group.
- License/legal terms: see `third_party/licenses/libjpeg-IJG-README.txt`
  (the IJG README included with the source).

## Other bundled FLTK code

FLTK also includes its `xutf8` text-shaping code (Jean-Marc Lienher / O'ksi'D and
Free Software Foundation contributors) under the FLTK license; its notice remains
in `third_party/fltk-1.3.11/src/xutf8/COPYING`.

## Licensing notes

- The FLTK exception permits static linking; Windows CMake builds link the
  vendored libraries statically, so no FLTK DLL needs to be shipped.
- Keep `third_party/fltk-1.3.11/COPYING` and the license files in
  `third_party/licenses/` when redistributing source or binaries.
- If you modify the vendored FLTK source, the LGPL/FLTK license terms (including
  marking modified files and offering source for the modified library) apply.
- The ArgMap application code itself is original to this project; this document
  does not assign it a separate open-source license.
