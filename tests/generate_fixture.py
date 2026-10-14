#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Generate the deterministic OFD fixtures of the test suite.

Layout follows the GB/T 33190-2016 structure used by the specification
examples (OFD.xml -> Doc_0/Document.xml -> Doc_0/Pages/Page_N/Content.xml, with
an optional Doc_0/Res.xml font catalog in Doc_0/CommonData/PublicRes):

    generate_fixture.py <kind> <output.ofd> [count]

Kinds:
  geometry       three pages with distinct boxes (A4/A5/landscape, 0 0 origin)
  corner-marker  A4 page with a filled black marker in the top-left corner
  origin-offset  A4 page whose PhysicalBox origin is "10 20": the marker covers
                 the box, so any rendered pixel proves the origin subtraction
  metadata       A4 page with ASCII and CJK text, plus every metadata field a
                 real invoice carries (dates in the date-only producer form)
  latency        <count> (default 300) empty A4 pages, for open-latency runs

Fixed timestamps keep every archive byte-identical across runs.
"""

import sys
import zipfile

A4 = (0.0, 0.0, 210.0, 297.0)

OFD_XML = """<?xml version="1.0" encoding="UTF-8"?>
<ofd:OFD xmlns:ofd="http://www.ofdspec.org/2016" DocType="OFD" Version="1.0">
  <ofd:DocBody>
    <ofd:DocInfo>
{info}    </ofd:DocInfo>
    <ofd:DocRoot>Doc_0/Document.xml</ofd:DocRoot>
  </ofd:DocBody>
</ofd:OFD>
"""

DOCUMENT_XML = """<?xml version="1.0" encoding="UTF-8"?>
<ofd:Document xmlns:ofd="http://www.ofdspec.org/2016">
  <ofd:CommonData>
    <ofd:MaxUnitID>{max_unit}</ofd:MaxUnitID>
    <ofd:PageArea>
      <ofd:PhysicalBox>{default_box}</ofd:PhysicalBox>
    </ofd:PageArea>
{public_res}  </ofd:CommonData>
  <ofd:Pages>
{pages}  </ofd:Pages>
</ofd:Document>
"""

PAGE_REF = '    <ofd:Page ID="{page_id}" BaseLoc="Pages/Page_{index}/Content.xml"/>\n'

PAGE_XML = """<?xml version="1.0" encoding="UTF-8"?>
<ofd:Page xmlns:ofd="http://www.ofdspec.org/2016">
  <ofd:Area>
    <ofd:PhysicalBox>{box}</ofd:PhysicalBox>
  </ofd:Area>
  <ofd:Content>
    <ofd:Layer ID="1">
{objects}    </ofd:Layer>
  </ofd:Content>
</ofd:Page>
"""

FONT_CATALOG = """  <ofd:PublicRes>Res.xml</ofd:PublicRes>
"""

RES_XML = """<?xml version="1.0" encoding="UTF-8"?>
<ofd:Res xmlns:ofd="http://www.ofdspec.org/2016">
  <ofd:Fonts>
    <ofd:Font ID="10" FontName="Fixture Sans"/>
  </ofd:Fonts>
</ofd:Res>
"""

CORNER_MARKER = """      <ofd:PathObject ID="20" Boundary="5 5 20 20" Fill="true">
        <ofd:FillColor Value="0 0 0"/>
        <ofd:AbbreviatedData>M 0 0 L 20 0 L 20 20 L 0 20 C</ofd:AbbreviatedData>
      </ofd:PathObject>
"""

BOX_FILL = """      <ofd:PathObject ID="20" Boundary="{boundary}" Fill="true">
        <ofd:FillColor Value="0 0 0"/>
        <ofd:AbbreviatedData>{data}</ofd:AbbreviatedData>
      </ofd:PathObject>
"""

TEXT_OBJECT = """      <ofd:TextObject ID="{id}" Boundary="{boundary}" Font="10" Size="6">
        <ofd:TextCode X="0" Y="6">{text}</ofd:TextCode>
      </ofd:TextObject>
"""


def box_string(box):
    return " ".join(f"{value:g}" for value in box)


def box_fill(box):
    """An object whose path covers the whole box (origin-relative data)."""
    width, height = box[2], box[3]
    data = f"M 0 0 L {width:g} 0 L {width:g} {height:g} L 0 {height:g} C"
    return BOX_FILL.format(boundary=box_string(box), data=data)


def text_object(object_id, boundary, text):
    return TEXT_OBJECT.format(id=object_id, boundary=boundary, text=text)


def build(path, pages, info="", font_catalog=False, max_unit=1):
    """pages: list of (box, objects-xml). info: DocInfo child lines."""
    document = DOCUMENT_XML.format(
        max_unit=max_unit,
        default_box=box_string(pages[0][0]),
        public_res=FONT_CATALOG if font_catalog else "",
        pages="".join(
            PAGE_REF.format(page_id=index + 1, index=index)
            for index in range(len(pages))
        ),
    )

    # Fixed timestamps keep the archive byte-identical across runs.
    stamp = (2026, 1, 1, 0, 0, 0)
    with zipfile.ZipFile(path, "w", zipfile.ZIP_DEFLATED) as archive:
        archive.writestr(zipfile.ZipInfo("OFD.xml", date_time=stamp),
                         OFD_XML.format(info=info))
        archive.writestr(zipfile.ZipInfo("Doc_0/Document.xml", date_time=stamp), document)
        for index, (box, objects) in enumerate(pages):
            name = f"Doc_0/Pages/Page_{index}/Content.xml"
            archive.writestr(zipfile.ZipInfo(name, date_time=stamp),
                             PAGE_XML.format(box=box_string(box), objects=objects))
        if font_catalog:
            archive.writestr(zipfile.ZipInfo("Doc_0/Res.xml", date_time=stamp), RES_XML)


def geometry_pages():
    return [
        ((0.0, 0.0, 210.0, 297.0), ""),   # A4 portrait
        ((0.0, 0.0, 148.0, 210.0), ""),   # A5 portrait
        ((0.0, 0.0, 297.0, 210.0), ""),   # A4 landscape
    ]


def metadata_info():
    return (
        "      <ofd:DocID>papers-ofd-metadata-fixture</ofd:DocID>\n"
        "      <ofd:Title>Phase 1 fixture</ofd:Title>\n"
        "      <ofd:Author>Papers OFD tests</ofd:Author>\n"
        "      <ofd:Subject>Metadata mapping</ofd:Subject>\n"
        "      <ofd:Creator>fixture-generator</ofd:Creator>\n"
        "      <ofd:CreatorVersion>1.0</ofd:CreatorVersion>\n"
        "      <ofd:CreationDate>2025-01-06</ofd:CreationDate>\n"
        "      <ofd:ModDate>2024-10-22</ofd:ModDate>\n"
        "      <ofd:Keywords><ofd:Keyword>alpha</ofd:Keyword>"
        "<ofd:Keyword/><ofd:Keyword>beta</ofd:Keyword></ofd:Keywords>\n"
    )


LATENCY_PAGE_OBJECTS = (
    "      <ofd:PathObject ID=\"20\" Boundary=\"10 10 190 8\" Fill=\"true\">\n"
    "        <ofd:FillColor Value=\"0 0 0\"/>\n"
    "        <ofd:AbbreviatedData>M 0 0 L 190 0 L 190 8 L 0 8 C</ofd:AbbreviatedData>\n"
    "      </ofd:PathObject>\n"
    "      <ofd:TextObject ID=\"21\" Boundary=\"20 40 170 8\" Font=\"10\" Size=\"5\">\n"
    "        <ofd:TextCode X=\"0\" Y=\"5\">Page heading for the latency fixture</ofd:TextCode>\n"
    "      </ofd:TextObject>\n"
    "      <ofd:TextObject ID=\"22\" Boundary=\"20 60 170 8\" Font=\"10\" Size=\"4\">\n"
    "        <ofd:TextCode X=\"0\" Y=\"4\">Body text with some glyphs 0123456789</ofd:TextCode>\n"
    "      </ofd:TextObject>\n"
)


def pages_for_kind(kind, count):
    if kind == "geometry":
        return geometry_pages(), "", False
    if kind == "corner-marker":
        return [(A4, CORNER_MARKER)], "", False
    if kind == "origin-offset":
        box = (10.0, 20.0, 210.0, 297.0)
        return [(box, box_fill(box))], "", False
    if kind == "metadata":
        objects = (
            text_object(21, "20 30 120 10", "Invoice 42")
            + text_object(22, "20 45 120 10", "发票编号 42")
        )
        return [(A4, objects)], metadata_info(), True
    if kind == "latency":
        return [(A4, LATENCY_PAGE_OBJECTS)] * count, "", True
    raise SystemExit(f"unknown fixture kind: {kind}")


def main():
    if len(sys.argv) not in (3, 4):
        print(f"usage: {sys.argv[0]} <kind> <output.ofd> [count]", file=sys.stderr)
        return 2

    kind = sys.argv[1]
    count = int(sys.argv[3]) if len(sys.argv) == 4 else 300
    pages, info, font_catalog = pages_for_kind(kind, count)
    build(sys.argv[2], pages, info=info, font_catalog=font_catalog,
          max_unit=2 + 10 * len(pages))
    return 0


if __name__ == "__main__":
    sys.exit(main())
