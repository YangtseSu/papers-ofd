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
  text           two pages of text: a two-column ASCII page and a CJK page
  scan           one page holding only a PNG image, no text objects
  links          two pages with page and object actions, plus an outline with
                 nested nodes and a named bookmark
  origin-text    one page whose PhysicalBox origin is "10 20", with a text
                 object and a link, for the origin-normalisation checks
  broken-page    two pages, the second one truncated mid-element
  malformed-zip  zip magic and "OFD.xml" at byte 30, but not a zip at all
  text           two A4 pages of TextObjects: an ASCII invoice and CJK rows
  scan           A4 page with one PNG ImageObject plus its Res.xml catalog
  links          two A4 pages with page/object actions, an outline, bookmarks
  broken-page    two pages whose page 1 Content.xml is truncated mid-element
  malformed-zip  a truncated ZIP carrying only the OFD.xml magic signature

Fixed timestamps keep every archive byte-identical across runs.
"""

import struct
import sys
import zipfile
import zlib

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
{public_res}{common_extra}  </ofd:CommonData>
  <ofd:Pages>
{pages}  </ofd:Pages>
{document_tail}</ofd:Document>
"""

PAGE_REF = '    <ofd:Page ID="{page_id}" BaseLoc="Pages/Page_{index}/Content.xml"/>\n'

PAGE_XML = """<?xml version="1.0" encoding="UTF-8"?>
<ofd:Page xmlns:ofd="http://www.ofdspec.org/2016"{page_attributes}>
  <ofd:Area>
    <ofd:PhysicalBox>{box}</ofd:PhysicalBox>
  </ofd:Area>
  <ofd:Content>
    <ofd:Layer ID="1">
{objects}    </ofd:Layer>
  </ofd:Content>
{page_extra}</ofd:Page>
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

TEXT_OBJECT = """      <ofd:TextObject ID="{id}" Boundary="{boundary}" Font="10" Size="{size}">
        <ofd:TextCode X="0" Y="{size}">{text}</ofd:TextCode>
      </ofd:TextObject>
"""

SCAN_IMAGE_OBJECT = (
    '      <ofd:ImageObject ID="51" Boundary="30 40 150 100" ResourceID="10"/>\n'
)

SCAN_DOCUMENT_RES = "  <ofd:DocumentRes>Res.xml</ofd:DocumentRes>\n"

SCAN_RES_XML = """<?xml version="1.0" encoding="UTF-8"?>
<ofd:Res xmlns:ofd="http://www.ofdspec.org/2016">
  <ofd:MultiMedias>
    <ofd:MultiMedia ID="10" Type="Image" Format="PNG">
      <ofd:MediaFile>Res/scan.png</ofd:MediaFile>
    </ofd:MultiMedia>
  </ofd:MultiMedias>
</ofd:Res>
"""

LINKS_PAGE_0_OBJECTS = """      <ofd:PathObject ID="71" Boundary="40 100 60 20" Fill="true">
        <ofd:FillColor Value="0 0 0"/>
        <ofd:AbbreviatedData>M 0 0 L 60 0 L 60 20 L 0 20 C</ofd:AbbreviatedData>
        <ofd:Actions>
          <ofd:Action Event="CLICK">
            <ofd:Goto>
              <ofd:Dest Type="Fit" PageID="2"/>
            </ofd:Goto>
          </ofd:Action>
        </ofd:Actions>
      </ofd:PathObject>
      <ofd:PathObject ID="72" Boundary="40 140 60 20" Fill="true">
        <ofd:FillColor Value="0 0 0"/>
        <ofd:AbbreviatedData>M 0 0 L 60 0 L 60 20 L 0 20 C</ofd:AbbreviatedData>
        <ofd:Actions>
          <ofd:Action Event="CLICK">
            <ofd:URI URI="relative/target" Base="https://example.test/base/"/>
          </ofd:Action>
        </ofd:Actions>
      </ofd:PathObject>
      <ofd:PathObject ID="73" Boundary="40 180 60 20" Fill="true">
        <ofd:FillColor Value="0 0 0"/>
        <ofd:AbbreviatedData>M 0 0 L 60 0 L 60 20 L 0 20 C</ofd:AbbreviatedData>
        <ofd:Actions>
          <!-- An unresolved GOTO first (no such page), a usable action second:
               the link must still be clickable (R11). -->
          <ofd:Action Event="CLICK">
            <ofd:Goto>
              <ofd:Dest Type="Fit" PageID="99"/>
            </ofd:Goto>
          </ofd:Action>
          <ofd:Action Event="CLICK">
            <ofd:URI URI="https://example.test/fallback-action"/>
          </ofd:Action>
        </ofd:Actions>
      </ofd:PathObject>
"""

LINKS_PAGE_0_ACTIONS = """  <ofd:Actions>
    <ofd:Action Event="CLICK">
      <ofd:URI URI="https://example.test/page-link"/>
    </ofd:Action>
  </ofd:Actions>
"""

LINKS_PAGE_1_ACTIONS = """  <ofd:Actions>
    <ofd:Action Event="CLICK">
      <ofd:Goto>
        <ofd:Dest Type="XYZ" PageID="1" Left="10" Top="20" Zoom="2"/>
      </ofd:Goto>
    </ofd:Action>
  </ofd:Actions>
"""

LINKS_DOCUMENT_TAIL = """  <ofd:Outlines>
    <ofd:OutlineElem ID="100" Title="第一章 概览" Expanded="false">
      <ofd:Actions>
        <ofd:Action Event="CLICK">
          <ofd:Goto>
            <ofd:Dest Type="Fit" PageID="1"/>
          </ofd:Goto>
        </ofd:Action>
      </ofd:Actions>
      <ofd:OutlineElem ID="101" Title="1.1 详情">
        <ofd:Actions>
          <ofd:Action Event="CLICK">
            <ofd:Goto>
              <ofd:Dest Type="FitH" PageID="2" Top="25"/>
            </ofd:Goto>
          </ofd:Action>
        </ofd:Actions>
      </ofd:OutlineElem>
    </ofd:OutlineElem>
    <ofd:OutlineElem ID="102" Title="网站">
      <ofd:Actions>
        <ofd:Action Event="CLICK">
          <ofd:URI URI="https://example.test/outline"/>
        </ofd:Action>
      </ofd:Actions>
    </ofd:OutlineElem>
    <ofd:OutlineElem ID="103" Title="书签目标">
      <ofd:Actions>
        <ofd:Action Event="CLICK">
          <ofd:Goto>
            <ofd:Bookmark Name="target"/>
          </ofd:Goto>
        </ofd:Action>
      </ofd:Actions>
    </ofd:OutlineElem>
    <ofd:OutlineElem ID="104" Title="附属内容">
      <!-- No action of its own: the row inherits the first reachable GOTO of its
           subtree, or the sidebar would read page -1 for it (R8). -->
      <ofd:OutlineElem ID="105" Title="4.1 附录">
        <ofd:Actions>
          <ofd:Action Event="CLICK">
            <ofd:Goto>
              <ofd:Dest Type="FitV" PageID="2" Left="15"/>
            </ofd:Goto>
          </ofd:Action>
        </ofd:Actions>
      </ofd:OutlineElem>
    </ofd:OutlineElem>
  </ofd:Outlines>
  <ofd:Bookmarks>
    <ofd:Bookmark Name="target">
      <ofd:Dest Type="XYZ" PageID="2" Left="10" Top="30" Zoom="1.5"/>
    </ofd:Bookmark>
  </ofd:Bookmarks>
"""

BROKEN_PAGE = (
    '<?xml version="1.0" encoding="UTF-8"?>'
    '<ofd:Page xmlns:ofd="http://www.ofdspec.org/2016">'
    '<ofd:Content><ofd:Layer ID="1"><ofd:TextOb'
)


def png_chunk(tag, data):
    crc = zlib.crc32(tag + data) & 0xFFFFFFFF
    return struct.pack(">I", len(data)) + tag + data + struct.pack(">I", crc)


def scan_png():
    """A deterministic 64x64 8-bit RGB PNG: red frame, 8px checkerboard."""
    width = height = 64
    raw = bytearray()
    for y in range(height):
        raw.append(0)  # filter type 0 (None)
        for x in range(width):
            if x < 4 or y < 4 or x >= width - 4 or y >= height - 4:
                pixel = (255, 0, 0)
            elif ((x // 8) + (y // 8)) % 2 == 0:
                pixel = (0, 0, 0)
            else:
                pixel = (255, 255, 255)
            raw.extend(pixel)
    ihdr = struct.pack(">IIBBBBB", width, height, 8, 2, 0, 0, 0)
    return (
        b"\x89PNG\r\n\x1a\n"
        + png_chunk(b"IHDR", ihdr)
        + png_chunk(b"IDAT", zlib.compress(bytes(raw), 9))
        + png_chunk(b"IEND", b"")
    )


def write_malformed_zip(path):
    """A truncated ZIP whose only magic is "OFD.xml" at byte offset 30."""
    with open(path, "wb") as handle:
        handle.write(b"PK\x03\x04" + bytes(26) + b"OFD.xml" + b"\x41" * 406)


def page(box, objects="", attributes="", extra=""):
    return (box, objects, attributes, extra)


def box_string(box):
    return " ".join(f"{value:g}" for value in box)


def box_fill(box):
    """An object whose path covers the whole box (origin-relative data)."""
    width, height = box[2], box[3]
    data = f"M 0 0 L {width:g} 0 L {width:g} {height:g} L 0 {height:g} C"
    return BOX_FILL.format(boundary=box_string(box), data=data)


def text_object(object_id, boundary, text, size=6):
    return TEXT_OBJECT.format(id=object_id, boundary=boundary, text=text, size=size)


def build(path, pages, info="", font_catalog=False, max_unit=1,
          common_extra="", document_tail="", extra_files=(), raw_pages=None):
    """pages: list of (box, objects-xml, page-attributes, page-extra)."""
    document = DOCUMENT_XML.format(
        max_unit=max_unit,
        default_box=box_string(pages[0][0]),
        public_res=FONT_CATALOG if font_catalog else "",
        common_extra=common_extra,
        pages="".join(
            PAGE_REF.format(page_id=index + 1, index=index)
            for index in range(len(pages))
        ),
        document_tail=document_tail,
    )

    # Fixed timestamps keep the archive byte-identical across runs.
    stamp = (2026, 1, 1, 0, 0, 0)
    with zipfile.ZipFile(path, "w", zipfile.ZIP_DEFLATED) as archive:
        archive.writestr(zipfile.ZipInfo("OFD.xml", date_time=stamp),
                         OFD_XML.format(info=info))
        archive.writestr(zipfile.ZipInfo("Doc_0/Document.xml", date_time=stamp), document)
        for index, (box, objects, attributes, extra) in enumerate(pages):
            name = f"Doc_0/Pages/Page_{index}/Content.xml"
            if raw_pages and index in raw_pages:
                data = raw_pages[index]
            else:
                data = PAGE_XML.format(box=box_string(box), objects=objects,
                                       page_attributes=attributes, page_extra=extra)
            archive.writestr(zipfile.ZipInfo(name, date_time=stamp), data)
        if font_catalog:
            archive.writestr(zipfile.ZipInfo("Doc_0/Res.xml", date_time=stamp), RES_XML)
        for name, data in extra_files:
            archive.writestr(zipfile.ZipInfo(name, date_time=stamp), data)


def geometry_pages():
    return [
        page((0.0, 0.0, 210.0, 297.0)),   # A4 portrait
        page((0.0, 0.0, 148.0, 210.0)),   # A5 portrait
        page((0.0, 0.0, 297.0, 210.0)),   # A4 landscape
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


def text_pages():
    # Size 3 keeps every line inside its column: rofd falls back to a uniform
    # advance per glyph when the named font is absent, so a line is about
    # 0.96 * size millimetres per character. The left column stays inside
    # x <= 112 mm and the right one inside 120..210 mm, which is what the
    # area and selection checks in test-ofd-text.c rely on.
    page_0 = (
        text_object(21, "15 25 90 8", "Invoice total: 42 EUR", size=3)
        + text_object(22, "15 37 90 8", "Payment received on Friday", size=3)
        + text_object(23, "15 49 90 8", "Reference INV-42-ALPHA", size=3)
        + text_object(31, "120 25 80 8", "Second column starts here", size=3)
        + text_object(32, "120 37 80 8", "invoice appears lowercase", size=3)
        + text_object(33, "120 49 80 8", "End of the second column", size=3)
    )
    page_1 = (
        text_object(41, "20 30 170 10", "发票编号 42", size=5)
        + text_object(42, "20 45 170 10", "中文搜索测试：发票编号 42 完成", size=5)
    )
    return [page(A4, page_0), page(A4, page_1)]


# A page whose PhysicalBox origin is not (0, 0): every rectangle the module
# converts must come back relative to that origin (plan §4, D3), which is
# invisible on the usual "0 0 w h" boxes.
ORIGIN_BOX = (10.0, 20.0, 210.0, 297.0)

ORIGIN_TEXT_OBJECTS = """      <ofd:TextObject ID="91" Boundary="10 20 210 20" Font="10" Size="4">
        <ofd:TextCode X="5" Y="6">Origin normalised text</ofd:TextCode>
      </ofd:TextObject>
      <ofd:PathObject ID="92" Boundary="10 100 60 20" Fill="true">
        <ofd:FillColor Value="0 0 0"/>
        <ofd:AbbreviatedData>M 0 0 L 60 0 L 60 20 L 0 20 C</ofd:AbbreviatedData>
        <ofd:Actions>
          <ofd:Action Event="CLICK">
            <ofd:Goto>
              <ofd:Dest Type="XYZ" PageID="1" Left="15" Top="26" Zoom="0"/>
            </ofd:Goto>
          </ofd:Action>
        </ofd:Actions>
      </ofd:PathObject>
"""


def links_pages():
    return [
        page(A4, LINKS_PAGE_0_OBJECTS, extra=LINKS_PAGE_0_ACTIONS),
        page(A4, text_object(61, "20 30 80 8", "Second page"),
             attributes=' ID="2"', extra=LINKS_PAGE_1_ACTIONS),
    ]


def fixture_spec(kind, count):
    """The build() keyword arguments for a fixture kind (plus an optional raw flag)."""
    if kind == "geometry":
        return {"pages": geometry_pages()}
    if kind == "corner-marker":
        return {"pages": [page(A4, CORNER_MARKER)]}
    if kind == "origin-offset":
        box = (10.0, 20.0, 210.0, 297.0)
        return {"pages": [page(box, box_fill(box))]}
    if kind == "metadata":
        objects = (
            text_object(21, "20 30 120 10", "Invoice 42")
            + text_object(22, "20 45 120 10", "发票编号 42")
        )
        return {"pages": [page(A4, objects)], "info": metadata_info(),
                "font_catalog": True}
    if kind == "latency":
        return {"pages": [page(A4, LATENCY_PAGE_OBJECTS)] * count,
                "font_catalog": True}
    if kind == "text":
        return {"pages": text_pages(), "max_unit": 42, "font_catalog": True}
    if kind == "scan":
        return {"pages": [page(A4, SCAN_IMAGE_OBJECT)], "max_unit": 51,
                "common_extra": SCAN_DOCUMENT_RES,
                "extra_files": [("Doc_0/Res.xml", SCAN_RES_XML),
                                ("Doc_0/Res/scan.png", scan_png())]}
    if kind == "origin-text":
        return {"pages": [page(ORIGIN_BOX, ORIGIN_TEXT_OBJECTS)],
                "max_unit": 92, "font_catalog": True}
    if kind == "links":
        return {"pages": links_pages(), "max_unit": 106, "font_catalog": True,
                "document_tail": LINKS_DOCUMENT_TAIL}
    if kind == "broken-page":
        return {"pages": [page(A4, text_object(81, "20 30 120 10",
                                               "Page zero text")),
                          page(A4, "")],
                "max_unit": 81, "font_catalog": True,
                "raw_pages": {1: BROKEN_PAGE.encode("utf-8")}}
    if kind == "strip":
        # A page whose width is 40x its height: the one request shape the render
        # budget cannot absorb, because the canvas crosses cairo's 32767-pixel
        # dimension limit long before the byte cap bites (R15's test drives a
        # failed render through it).
        return {"pages": [page((0.0, 0.0, 1000.0, 25.0), "")]}
    if kind == "malformed-zip":
        return {"raw": True}
    raise SystemExit(f"unknown fixture kind: {kind}")


def main():
    if len(sys.argv) not in (3, 4):
        print(f"usage: {sys.argv[0]} <kind> <output.ofd> [count]", file=sys.stderr)
        return 2

    kind = sys.argv[1]
    count = int(sys.argv[3]) if len(sys.argv) == 4 else 300
    spec = fixture_spec(kind, count)
    if spec.pop("raw", False):
        write_malformed_zip(sys.argv[2])
        return 0
    spec.setdefault("max_unit", 2 + 10 * len(spec["pages"]))
    build(sys.argv[2], **spec)
    return 0


if __name__ == "__main__":
    sys.exit(main())
