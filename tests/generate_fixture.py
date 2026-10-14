#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Generate a deterministic three-page OFD with distinct page boxes.

Layout follows the GB/T 33190-2016 structure used by the specification
examples (OFD.xml -> Doc_0/Document.xml -> Doc_0/Pages/Page_N/Content.xml).
Only geometry is emitted: the pages have no content objects, which is enough
for the phase-0 page-count/page-size checks.
"""

import sys
import zipfile

PAGES = [
    # (PhysicalBox x y w h) millimetres
    (0.0, 0.0, 210.0, 297.0),   # A4 portrait
    (0.0, 0.0, 148.0, 210.0),   # A5 portrait
    (0.0, 0.0, 297.0, 210.0),   # A4 landscape
]

OFD_XML = """<?xml version="1.0" encoding="UTF-8"?>
<ofd:OFD xmlns:ofd="http://www.ofdspec.org/2016" DocType="OFD" Version="1.0">
  <ofd:DocBody>
    <ofd:DocInfo>
      <ofd:DocID>papers-ofd-test-fixture</ofd:DocID>
    </ofd:DocInfo>
    <ofd:DocRoot>Doc_0/Document.xml</ofd:DocRoot>
  </ofd:DocBody>
</ofd:OFD>
"""

DOCUMENT_XML_HEAD = """<?xml version="1.0" encoding="UTF-8"?>
<ofd:Document xmlns:ofd="http://www.ofdspec.org/2016">
  <ofd:CommonData>
    <ofd:MaxUnitID>1</ofd:MaxUnitID>
    <ofd:PageArea>
      <ofd:PhysicalBox>{default_box}</ofd:PhysicalBox>
    </ofd:PageArea>
  </ofd:CommonData>
  <ofd:Pages>
"""

DOCUMENT_XML_PAGE = '    <ofd:Page ID="{page_id}" BaseLoc="Pages/Page_{index}/Content.xml"/>\n'

DOCUMENT_XML_TAIL = """  </ofd:Pages>
</ofd:Document>
"""

PAGE_XML = """<?xml version="1.0" encoding="UTF-8"?>
<ofd:Page xmlns:ofd="http://www.ofdspec.org/2016">
  <ofd:Area>
    <ofd:PhysicalBox>{box}</ofd:PhysicalBox>
  </ofd:Area>
  <ofd:Content>
    <ofd:Layer ID="1"/>
  </ofd:Content>
</ofd:Page>
"""


def box_string(box):
    return " ".join(f"{value:g}" for value in box)


def build(path):
    document = DOCUMENT_XML_HEAD.format(default_box=box_string(PAGES[0]))
    for index, box in enumerate(PAGES):
        document += DOCUMENT_XML_PAGE.format(page_id=index + 1, index=index)
    document += DOCUMENT_XML_TAIL

    # Fixed timestamps keep the archive byte-identical across runs.
    info = zipfile.ZipInfo("OFD.xml", date_time=(2026, 1, 1, 0, 0, 0))

    with zipfile.ZipFile(path, "w", zipfile.ZIP_DEFLATED) as archive:
        archive.writestr(info, OFD_XML)
        archive.writestr("Doc_0/Document.xml", document)
        for index, box in enumerate(PAGES):
            name = f"Doc_0/Pages/Page_{index}/Content.xml"
            archive.writestr(name, PAGE_XML.format(box=box_string(box)))


def main():
    if len(sys.argv) != 2:
        print(f"usage: {sys.argv[0]} <output.ofd>", file=sys.stderr)
        return 2
    build(sys.argv[1])
    return 0


if __name__ == "__main__":
    sys.exit(main())
