# Third-party notices

Vibmory's own code uses the MIT license in LICENSE. Bundled dependencies keep
their own copyrights and licenses. The application uses shared Qt 6.8.3
libraries (Core, Gui, Widgets, Network, Multimedia, Svg and required plugins)
under applicable LGPLv3 and third-party terms. Qt is maintained by The Qt
Company and other contributors.

This software uses libraries from the FFmpeg project under LGPLv2.1 or later.
The packaged build checks FFmpeg 7.1 and its actual license/configuration.
Its source archive is supplied in the companion Vibmory-dependency-sources
artifact alongside the executable. Qt's complete relevant module sources,
upstream notices, and software-renderer sources are supplied there as well.

Windows packages also contain the optional Mesa/LLVM software renderer,
Microsoft graphics redistributables, and the Microsoft C++ release runtime.
Their licenses are independent of Vibmory's MIT license. See
docs/Distribution.md for component terms, supplier links, source/replacement
instructions, and redistribution details. No application term restricts
replacement of LGPL libraries or reverse engineering needed to debug them.

The package's licenses/ directory contains full upstream license/notice files,
source attribution metadata and vendor notices. runtime-inventory.json records
the actual shipped binaries, hashes, versions and component assignments.

The legacy `Helper` implementation was adapted from Qt's examples. Its original
notice is preserved below even as that implementation is replaced.

```text
/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/
```
