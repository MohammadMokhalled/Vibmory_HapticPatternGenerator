# Distribution, licenses, and dependency sources

Project-owned Vibmory code is provided under the [MIT license](../LICENSE), copyright 2021-2026 Mohammad Mokhalled and contributors. Third-party components retain their own terms. The original Qt example notice is preserved in [THIRD_PARTY_NOTICES](../THIRD_PARTY_NOTICES.md).

The supported binary distribution is Windows x64. The application uses shared Qt libraries; users can replace compatible DLLs/plugins and rebuild Vibmory from source. The project imposes no restriction on reverse engineering needed to debug changes to those libraries. Keep the application architecture, Qt major/minor compatibility, plugin interfaces, and compiler runtime compatible when replacing libraries. Build instructions are in README; Qt's [source build instructions](https://doc.qt.io/qt-6.8/build-sources.html) cover rebuilding Qt.

## What accompanies a binary

The Windows build contains:

- `LICENSE`, `THIRD_PARTY_NOTICES.md`, offline Markdown documentation, and `application.json` generated from CMake's application name/version/source revision.
- `runtime-inventory.json`: each deployed executable, DLL, and Qt translation catalog, with its component, SHA-256, and PE file version when supplied by the vendor. Unrecognized binary dependencies stop packaging for review.
- `licenses/`: upstream license/copyright/notice files, referenced Qt third-party license files, source attribution metadata, pinned DirectX Shader Compiler notices, available relevant Qt SDK SBOMs, and the actual FFmpeg license/configuration/version read from the deployed libraries.
- A companion **Vibmory-dependency-sources** artifact containing the pinned Qt/FFmpeg upstream source archives, upstream Mesa/LLVM sources for the optional software renderer, provenance/checksums, and these replacement/build directions. It is uploaded alongside the binary with the same 30-day retention. Preserve the pair when redistributing or publishing a release.

`licenses/upstream-attributions.json` and collected source-module notices are a superset of possible features in those upstream modules. They do not assert that every source component is compiled into the application. The file-level runtime inventory describes what is actually shipped. The vendored licenses do not relicense Vibmory or other independent components.

## Components

| Component | Delivery / terms | Source and attribution |
| --- | --- | --- |
| Qt 6.8.3 Core, Gui, Widgets, Network, Multimedia, Svg and plugins/translations | Shared libraries/catalogs; applicable Qt LGPLv3 terms and individual third-party terms | Pinned qtbase, qtmultimedia, qtsvg, qttranslations archives and their complete upstream notice metadata; [Qt licensing](https://doc.qt.io/qt-6.8/licensing.html) |
| FFmpeg 7.1 | Qt's shared LGPL 2.1-or-later build; packaging checks its actual version/license/configuration and rejects GPL/nonfree build flags | Pinned `n7.1` source; Qt's [versioned attribution/build provenance](https://github.com/qt/qtmultimedia/blob/v6.8.3/src/3rdparty/ffmpeg/qt_attribution.json); [upstream licensing](https://ffmpeg.org/legal.html) |
| Mesa 11.2.2 / LLVM 3.6.2 software OpenGL | Optional `opengl32sw.dll`; upstream permissive notices, including MIT/BSL and LLVM/NCSA terms | Included upstream sources/notices; Qt's [software renderer build description](https://wiki.qt.io/MesaLlvmpipe) documents the vendor build/patches |
| Microsoft DirectX Shader Compiler / DXIL | Unmodified vendor `dxcompiler.dll` / `dxil.dll`; supplier notices and terms | Pinned Microsoft [license and notices](https://github.com/microsoft/DirectXShaderCompiler/tree/v1.9.2602); binary versions recorded separately |
| Microsoft Windows SDK D3D compiler | Unmodified `d3dcompiler_47.dll`; Microsoft Windows SDK terms | Supplied by Qt's deployment tool from the Windows SDK/system; [Windows SDK](https://developer.microsoft.com/en-us/windows/downloads/windows-sdk/); actual binary version/hash recorded |
| Microsoft Visual C++ runtime | Unmodified release redistributables in the application folder, with the vendor redistributable installer when deployed by Qt | [Runtime license directory](https://visualstudio.microsoft.com/license-terms/) and [Visual Studio 2026 redistribution list](https://learn.microsoft.com/en-us/visualstudio/releases/2026/redistribution); actual binary versions/hashes recorded |
| NSIS 3.13 installer | Installer bootstrap with the zlib compressor; supplier notices copied to `licenses/vendor/nsis-COPYING.txt` | Compiler archive/version/hash in `packaging/build_tools.json`; [upstream download/source](https://nsis.sourceforge.io/Download) |

Microsoft components are not covered by Vibmory's MIT license. A redistribution must retain their applicable supplier terms and use the licensed release redistributables. The application does not copy debug/nonredistributable compiler files. The inventory records changing hosted-toolchain versions rather than labeling them as Qt's compiler-kit version.

## Preparing or verifying a package

After `cmake --install` and release-runtime deployment on Windows:

```powershell
python tools/prepare_distribution.py --app-dir stage/Vibmory --sources-dir artifacts/dependency-sources --qt-root $env:QT_ROOT_DIR
python tools/prepare_distribution.py --app-dir stage/Vibmory --sources-dir artifacts/dependency-sources --verify
```

Source URLs and SHA-256 pins are in `packaging/source_archives.json`; small vendor-notice pins are in `packaging/notice_sources.json`. Downloads are cached under `out/distribution-sources`, checked before use, and never executed. `--offline` uses only already verified cached inputs. Archive extraction only copies bounded notice files inside the output directory. Symlinks are not extracted, and paths escaping that directory are rejected.

The project does not modify Qt, FFmpeg, Mesa, or LLVM sources. Qt supplies its binary build/configuration; FFmpeg's actual configure line is included in `PROVENANCE.json` and `licenses/ffmpeg-build.json`. The upstream Qt [Windows FFmpeg provisioning script](https://github.com/qt/qt5/blob/v6.8.3/coin/provisioning/common/windows/install-ffmpeg.ps1) and its [shared configure options](https://github.com/qt/qt5/blob/v6.8.3/coin/provisioning/common/shared/ffmpeg_config_options.txt) describe that vendor build. Mesa's optional prebuilt renderer is Qt's vendor build; the upstream Mesa/LLVM archives and Qt's linked build description are supplied for attribution and study, without claiming those pristine archives reproduce Qt's patched DLL byte-for-byte.
