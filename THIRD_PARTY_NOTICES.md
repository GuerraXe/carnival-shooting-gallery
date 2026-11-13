# Third-party notices — Carnival Shooting Gallery

The game executable is statically linked against the libraries below. Their full license texts ship with the game in `licenses/third-party/`; the build copies them verbatim from the pinned dependency sources (`cmake/ThirdPartyNotices.cmake`).

Portions of this software are copyright © 2023 The FreeType Project (www.freetype.org). All rights reserved.

| Component | Version | License | File in `licenses/third-party/` |
|---|---|---|---|
| SFML (Simple and Fast Multimedia Library) | 3.0.2 | zlib/png license | `SFML-license.md` |
| FreeType | 2.13.2 | FreeType License (FTL), chosen of FTL / GPLv2 | `FreeType-LICENSE.txt`, `FreeType-FTL.txt` |
| libogg | 1.3.5 | BSD 3-clause (Xiph.Org) | `libogg-COPYING.txt` |
| libvorbis | 1.3.7 | BSD 3-clause (Xiph.Org) | `libvorbis-COPYING.txt` |
| libFLAC | 1.4.3 | BSD 3-clause (Xiph.Org) | `FLAC-COPYING.Xiph.txt` |
| miniaudio (vendored in SFML) | as in SFML 3.0.2 | Public domain (Unlicense) or MIT No Attribution | `miniaudio-LICENSE.txt` |
| stb_image (vendored in SFML) | as in SFML 3.0.2 | MIT or public domain | `stb_image-LICENSE.txt` |
| minimp3 (vendored in SFML) | as in SFML 3.0.2 | CC0 1.0 | `minimp3-LICENSE.txt` |
| glad / Khronos `khrplatform.h` (vendored in SFML) | as in SFML 3.0.2 | Khronos MIT-style license | `glad-khrplatform-LICENSE.txt` |

The distribution build links the Microsoft C/C++ runtime statically (Visual Studio 2022 Build Tools, MSVC 14.44), so no Visual C++ Redistributable is needed.

## Game assets

Every sound in `assets/audio/` is a real recording released under CC0 1.0 (no attribution required). They are credited anyway: see the README's "Asset licensing and attribution" section, `assets/ASSET_MANIFEST.md`, and the per-asset evidence files in `assets/licenses/`. The CC0 legal code is `assets/licenses/CC0-1.0.txt`.
