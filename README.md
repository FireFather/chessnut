# chessnut
awesome uci chess engine
forked from https://github.com/AriaKafie/chessnut
[![Release][release-badge]][release-link]
[![Commits][commits-badge]][commits-link]

![Downloads][downloads-badge]
[![Forks][forks-shield]][forks-url]
[![Stargazers][stars-shield]][stars-url]

[![License][license-badge]][license-link]
[![Contributors][contributors-shield]][contributors-url]
[![Issues][issues-shield]][issues-url]

Xiphos chess engine (https://github.com/milostatarevic/xiphos)

- Ported from C to C++ and modernized
- Minimal: only the core chess engine remains
- Clean: warning-free at strict Level 4 compiler levels
- Modern: idiomatic, tool-friendly C++

```
perft 7
a2a3: 106743106
b2b3: 133233975
c2c3: 144074944
d2d3: 227598692
e2e3: 306138410
f2f3: 102021008
g2g3: 135987651
h2h3: 106678423
a2a4: 137077337
b2b4: 134087476
c2c4: 157756443
d2d4: 269605599
e2e4: 309478263
f2f4: 119614841
g2g4: 130293018
h2h4: 138495290
b1a3: 120142144
b1c3: 148527161
g1f3: 147678554
g1h3: 120669525
Nodes: 3195901860
Time: 4266ms
NPS: 749156554
```
## ⚙️To Build
- Visual Studio -> use the included project files: chessnut.sln or chessnut.vcxproj
- MSYS2 mingw64 -> use the included shell script: make_it.sh

[contributors-url]: https://github.com/FireFather/chessnut/graphs/contributors
[forks-url]: https://github.com/FireFather/chessnut/network/members
[stars-url]: https://github.com/FireFather/chessnut/stargazers
[issues-url]: https://github.com/FireFather/chessnut/issues

[contributors-shield]: https://img.shields.io/github/contributors/FireFather/chessnut?style=for-the-badge&color=blue
[forks-shield]: https://img.shields.io/github/forks/FireFather/chessnut?style=for-the-badge&color=blue
[stars-shield]: https://img.shields.io/github/stars/FireFather/chessnut?style=for-the-badge&color=blue
[issues-shield]: https://img.shields.io/github/issues/FireFather/chessnut?style=for-the-badge&color=blue

[license-badge]: https://img.shields.io/github/license/FireFather/chessnut?style=for-the-badge&label=license&color=blue
[license-link]: https://github.com/FireFather/chessnut/blob/main/LICENSE
[release-badge]: https://img.shields.io/github/v/release/FireFather/chessnut?style=for-the-badge&label=official%20release
[release-link]: https://github.com/FireFather/chessnut/releases/latest
[commits-badge]: https://img.shields.io/github/commits-since/FireFather/chessnut/latest?style=for-the-badge
[commits-link]: https://github.com/FireFather/chessnut/commits/main
[downloads-badge]: https://img.shields.io/github/downloads/FireFather/chessnut/total?style=for-the-badge&color=blue
