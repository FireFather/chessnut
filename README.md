# chessnut
awesome uci chess engine

[![Release][release-badge]][release-link]
[![Commits][commits-badge]][commits-link]

![Downloads][downloads-badge]
[![Forks][forks-shield]][forks-url]
[![Stargazers][stars-shield]][stars-url]

[![License][license-badge]][license-link]
[![Contributors][contributors-shield]][contributors-url]
[![Issues][issues-shield]][issues-url]

This is the modernized Chessnut chess engine from (https://github.com/AriaKafie/chessnut)

- updated: modernized C++
- Minimal: only the core chess engine remains
- Clean: warning-free at strict Level 4 compiler levels
- Modern: idiomatic, tool-friendly C++

```
perft 7
1 h2h3 106678423
2 g2g3 135987651
3 f2f3 102021008
4 e2e3 306138410
5 d2d3 227598692
6 c2c3 144074944
7 b2b3 133233975
8 a2a3 106743106
9 h2h4 138495290
10 g2g4 130293018
11 f2f4 119614841
12 e2e4 309478263
13 d2d4 269605599
14 c2c4 157756443
15 b2b4 134087476
16 a2a4 137077337
17 g1h3 120669525
18 g1f3 147678554
19 b1c3 148527161
20 b1a3 120142144
nodes 3195901860
time 5856 ms
```
Wow!!

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
