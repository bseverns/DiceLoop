#!/usr/bin/env python3
"""
Generate a logical wiring schematic for the DiceLoop hardware.

Outputs:
  - hardware/wiring/dice-loop-schematic.svg
  - hardware/wiring/dice-loop-schematic.md
  - hardware/wiring/dice-loop-schematic.kicad_sch
"""

from __future__ import annotations

import argparse
import html
import uuid
from dataclasses import dataclass
from pathlib import Path
from typing import Dict, Iterable, List, Tuple


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_SVG = ROOT / "hardware" / "wiring" / "dice-loop-schematic.svg"
DEFAULT_MD = ROOT / "hardware" / "wiring" / "dice-loop-schematic.md"
DEFAULT_KICAD = ROOT / "hardware" / "wiring" / "dice-loop-schematic.kicad_sch"


@dataclass(frozen=True)
class Block:
    block_id: str
    x: int
    y: int
    w: int
    h: int
    title: str
    lines: List[str]

    def right_anchor(self, y_offset: int = 0) -> Tuple[int, int]:
        return (self.x + self.w, self.y + self.h // 2 + y_offset)

    def left_anchor(self, y_offset: int = 0) -> Tuple[int, int]:
        return (self.x, self.y + self.h // 2 + y_offset)

    def top_anchor(self, x_offset: int = 0) -> Tuple[int, int]:
        return (self.x + self.w // 2 + x_offset, self.y)


@dataclass(frozen=True)
class Wire:
    start: Tuple[int, int]
    end: Tuple[int, int]
    label: str
    label_offset: Tuple[int, int] = (0, -8)


@dataclass(frozen=True)
class KicadSymbol:
    name: str
    ref_prefix: str
    value: str
    pins: List[str]


@dataclass(frozen=True)
class KicadInstance:
    symbol_name: str
    reference: str
    x: float
    y: float


def svg_header(width: int, height: int) -> str:
    return (
        f'<svg xmlns="http://www.w3.org/2000/svg" width="{width}" '
        f'height="{height}" viewBox="0 0 {width} {height}">'
    )


def svg_block(block: Block) -> str:
    title_y = block.y + 24
    lines_y = block.y + 50
    lines = []
    for idx, line in enumerate(block.lines):
        y = lines_y + idx * 18
        lines.append(
            f'<text x="{block.x + 16}" y="{y}" '
            f'font-family="monospace" font-size="13" fill="#172B4D">'
            f"{html.escape(line)}</text>"
        )
    return "\n".join(
        [
            f'<rect x="{block.x}" y="{block.y}" width="{block.w}" height="{block.h}" '
            'rx="12" ry="12" fill="#F4F6FB" stroke="#1E2A3A" stroke-width="2"/>',
            f'<text x="{block.x + 16}" y="{title_y}" '
            'font-family="monospace" font-size="15" font-weight="bold" '
            'fill="#0B1A2B">'
            f"{html.escape(block.title)}</text>",
            *lines,
        ]
    )


def svg_wire(wire: Wire) -> str:
    x1, y1 = wire.start
    x2, y2 = wire.end
    label_x = (x1 + x2) // 2 + wire.label_offset[0]
    label_y = (y1 + y2) // 2 + wire.label_offset[1]
    return "\n".join(
        [
            f'<line x1="{x1}" y1="{y1}" x2="{x2}" y2="{y2}" '
            'stroke="#D1495B" stroke-width="2"/>',
            f'<text x="{label_x}" y="{label_y}" font-family="monospace" '
            'font-size="12" fill="#6B1F2B">'
            f"{html.escape(wire.label)}</text>",
        ]
    )


def svg_text(x: int, y: int, text: str, size: int = 16, weight: str = "normal") -> str:
    return (
        f'<text x="{x}" y="{y}" font-family="monospace" '
        f'font-size="{size}" font-weight="{weight}" fill="#0B1A2B">'
        f"{html.escape(text)}</text>"
    )


def kfmt(value: float) -> str:
    return f"{value:.2f}"


def kicad_uuid() -> str:
    return str(uuid.uuid4())


def kicad_pin_offsets(pins: List[str]) -> List[Tuple[str, float]]:
    pin_pitch = 2.54
    y_start = (len(pins) - 1) * pin_pitch / 2.0
    return [(pin, y_start - idx * pin_pitch) for idx, pin in enumerate(pins)]


def render_kicad_symbol(symbol: KicadSymbol) -> str:
    pin_pitch = 2.54
    pin_count = len(symbol.pins)
    y_start = (pin_count - 1) * pin_pitch / 2.0
    top = y_start + 2.54
    bottom = y_start - (pin_count - 1) * pin_pitch - 2.54
    left = -1.27
    right = 22.86
    lines = [
        f'\t\t(symbol "{symbol.name}"',
        "\t\t\t(pin_names",
        "\t\t\t\t(offset 1.016)",
        "\t\t\t)",
        "\t\t\t(pin_numbers",
        "\t\t\t\t(offset 1.016)",
        "\t\t\t)",
        "\t\t\t(exclude_from_sim no)",
        "\t\t\t(in_bom yes)",
        "\t\t\t(on_board yes)",
        f'\t\t\t(property "Reference" "{symbol.ref_prefix}"',
        "\t\t\t\t(at 0 2.54 0)",
        "\t\t\t\t(effects",
        "\t\t\t\t\t(font",
        "\t\t\t\t\t\t(size 1.27 1.27)",
        "\t\t\t\t\t)",
        "\t\t\t\t)",
        "\t\t\t)",
        f'\t\t\t(property "Value" "{symbol.value}"',
        "\t\t\t\t(at 0 -2.54 0)",
        "\t\t\t\t(effects",
        "\t\t\t\t\t(font",
        "\t\t\t\t\t\t(size 1.27 1.27)",
        "\t\t\t\t\t)",
        "\t\t\t\t)",
        "\t\t\t)",
        '\t\t\t(property "Footprint" ""',
        "\t\t\t\t(at 0 0 0)",
        "\t\t\t\t(effects",
        "\t\t\t\t\t(font",
        "\t\t\t\t\t\t(size 1.27 1.27)",
        "\t\t\t\t\t)",
        "\t\t\t\t\t(hide yes)",
        "\t\t\t\t)",
        "\t\t\t)",
        '\t\t\t(property "Datasheet" ""',
        "\t\t\t\t(at 0 0 0)",
        "\t\t\t\t(effects",
        "\t\t\t\t\t(font",
        "\t\t\t\t\t\t(size 1.27 1.27)",
        "\t\t\t\t\t)",
        "\t\t\t\t\t(hide yes)",
        "\t\t\t\t)",
        "\t\t\t)",
        '\t\t\t(property "Description" ""',
        "\t\t\t\t(at 0 0 0)",
        "\t\t\t\t(effects",
        "\t\t\t\t\t(font",
        "\t\t\t\t\t\t(size 1.27 1.27)",
        "\t\t\t\t\t)",
        "\t\t\t\t\t(hide yes)",
        "\t\t\t\t)",
        "\t\t\t)",
        f'\t\t\t(symbol "{symbol.value}_1_1"',
        "\t\t\t\t(rectangle",
        f"\t\t\t\t\t(start {kfmt(left)} {kfmt(top)})",
        f"\t\t\t\t\t(end {kfmt(right)} {kfmt(bottom)})",
        "\t\t\t\t\t(stroke",
        "\t\t\t\t\t\t(width 0.254)",
        "\t\t\t\t\t\t(type default)",
        "\t\t\t\t\t)",
        "\t\t\t\t\t(fill",
        "\t\t\t\t\t\t(type background)",
        "\t\t\t\t\t)",
        "\t\t\t\t)",
    ]
    for idx, (pin, y) in enumerate(kicad_pin_offsets(symbol.pins), start=1):
        lines.extend(
            [
                "\t\t\t\t(pin passive line",
                f"\t\t\t\t\t(at -5.08 {kfmt(y)} 0)",
                "\t\t\t\t\t(length 3.81)",
                f'\t\t\t\t\t(name "{pin}"',
                "\t\t\t\t\t\t(effects",
                "\t\t\t\t\t\t\t(font",
                "\t\t\t\t\t\t\t\t(size 1.27 1.27)",
                "\t\t\t\t\t\t\t)",
                "\t\t\t\t\t\t)",
                "\t\t\t\t\t)",
                f'\t\t\t\t\t(number "{idx}"',
                "\t\t\t\t\t\t(effects",
                "\t\t\t\t\t\t\t(font",
                "\t\t\t\t\t\t\t\t(size 1.27 1.27)",
                "\t\t\t\t\t\t\t)",
                "\t\t\t\t\t\t)",
                "\t\t\t\t\t)",
                "\t\t\t\t)",
            ]
        )
    lines.extend(
        [
            "\t\t\t)",
            "\t\t\t(embedded_fonts no)",
            "\t\t)",
        ]
    )
    return "\n".join(lines)


def build_blocks(include_oled: bool) -> List[Block]:
    blocks = [
        Block(
            "teensy",
            420,
            260,
            320,
            240,
            "Teensy 4.0",
            [
                "A0 A1 A3 A4 A5 (pots)",
                "D2 D3 D4 (74HC595)",
                "D7 D8 (chaos buttons)",
                "D9 (entropy clock out)",
                "D5 D6 (OLED RES/DC)",
                "SDA 18 / SCL 19 (I2C)",
                "3.3V, GND",
            ],
        ),
        Block(
            "audio",
            420,
            40,
            320,
            150,
            "PJRC Audio Shield",
            [
                "I2S header stack",
                "MCLK -> pin 23",
                "SCK -> pin 13",
                "SDA/SCL shared",
                "Line in/out + SD",
            ],
        ),
        Block(
            "pots",
            820,
            220,
            320,
            150,
            "Control Pots (x5)",
            [
                "Gain / Loop / Feedback",
                "Dice / Chaos (10k lin)",
                "3.3V -> pot high",
                "GND -> pot low",
                "Wipers -> A0/A1/A3/A4/A5",
            ],
        ),
        Block(
            "buttons",
            820,
            390,
            320,
            110,
            "Chaos Buttons (x2)",
            [
                "Momentary to GND",
                "D7, D8 (pull-ups)",
            ],
        ),
        Block(
            "leds",
            820,
            520,
            320,
            150,
            "LED Bar + 74HC595",
            [
                "SER D2 / RCLK D3",
                "SRCLK D4",
                "QA-QH -> segments",
                "330R per segment",
            ],
        ),
    ]
    if include_oled:
        blocks.append(
            Block(
                "oled",
                820,
                690,
                320,
                130,
                "SSD1306 OLED (optional)",
                [
                    "SDA 18 / SCL 19",
                    "RES D5 / DC D6",
                    "3.3V, GND",
                ],
            )
        )
    return blocks


def build_wires(blocks: List[Block], include_oled: bool) -> List[Wire]:
    by_id = {b.block_id: b for b in blocks}
    wires = [
        Wire(
            by_id["teensy"].top_anchor(),
            by_id["audio"].left_anchor(),
            "I2S + MCLK 23 + SCK 13 + SDA/SCL",
            (-20, -10),
        ),
        Wire(
            by_id["teensy"].right_anchor(-40),
            by_id["pots"].left_anchor(-20),
            "A0/A1/A3/A4/A5 + 3.3V + GND",
        ),
        Wire(
            by_id["teensy"].right_anchor(20),
            by_id["buttons"].left_anchor(0),
            "D7/D8 + GND",
        ),
        Wire(
            by_id["teensy"].right_anchor(90),
            by_id["leds"].left_anchor(0),
            "D2/D3/D4 + 3.3V + GND",
        ),
    ]
    if include_oled:
        wires.append(
            Wire(
                by_id["teensy"].right_anchor(150),
                by_id["oled"].left_anchor(0),
                "SDA/SCL + D5/D6 + 3.3V + GND",
            )
        )
    return wires


def render_svg(blocks: Iterable[Block], wires: Iterable[Wire]) -> str:
    width, height = 1200, 880
    parts = [
        svg_header(width, height),
        '<rect x="0" y="0" width="1200" height="880" fill="#FDF8F4"/>',
        svg_text(40, 48, "DiceLoop hardware schematic (logical)", size=20, weight="bold"),
        svg_text(40, 76, "Generated from repo wiring docs", size=13),
    ]
    for wire in wires:
        parts.append(svg_wire(wire))
    for block in blocks:
        parts.append(svg_block(block))
    parts.append("</svg>")
    return "\n".join(parts)


def render_md(include_oled: bool) -> str:
    oled_note = "Included" if include_oled else "Omitted"
    rows = [
        "| Teensy 4.0 | Control pots | A0/A1/A3/A4/A5, 3.3V, GND |",
        "| Teensy 4.0 | Chaos buttons | D7, D8, GND (internal pull-ups) |",
        "| Teensy 4.0 | LED bar + 74HC595 | D2 (SER), D3 (RCLK), D4 (SRCLK), 3.3V, GND |",
        "| Teensy 4.0 | Audio shield | I2S stack, MCLK 23, SCK 13, SDA/SCL |",
    ]
    if include_oled:
        rows.append(
            "| Teensy 4.0 | SSD1306 OLED | SDA 18, SCL 19, D5 (RES), D6 (DC), 3.3V, GND |"
        )
    return "\n".join(
        [
            "# DiceLoop Schematic Notes",
            "",
            "This file is generated by `scripts/generate_schematic.py`.",
            "",
            f"- OLED: {oled_note}",
            "",
            "## Key Connections",
            "",
            "| From | To | Signals |",
            "| --- | --- | --- |",
            *rows,
            "",
            "## Notes",
            "",
            "- Pots are 10k linear, 0-3.3 V swing.",
            "- Buttons short to ground; Teensy uses internal pull-ups.",
            "- Shift register outputs need 330R current-limiting resistors.",
        ]
    )


def build_kicad_symbols(include_oled: bool) -> List[KicadSymbol]:
    symbols = [
        KicadSymbol(
            name="DiceLoop:Teensy_4_0",
            ref_prefix="U",
            value="Teensy_4_0",
            pins=[
                "3V3",
                "GND",
                "A0",
                "A1",
                "A3",
                "A4",
                "A5",
                "D2",
                "D3",
                "D4",
                "D5",
                "D6",
                "D7",
                "D8",
                "D9",
                "SDA",
                "SCL",
                "MCLK",
                "SCK",
                "I2S",
            ],
        ),
        KicadSymbol(
            name="DiceLoop:Audio_Shield",
            ref_prefix="U",
            value="Audio_Shield",
            pins=[
                "MCLK",
                "SCK",
                "SDA",
                "SCL",
                "I2S",
            ],
        ),
        KicadSymbol(
            name="DiceLoop:Control_Pots",
            ref_prefix="J",
            value="Control_Pots",
            pins=[
                "3V3",
                "GND",
                "A0",
                "A1",
                "A3",
                "A4",
                "A5",
            ],
        ),
        KicadSymbol(
            name="DiceLoop:Chaos_Buttons",
            ref_prefix="SW",
            value="Chaos_Buttons",
            pins=[
                "D7",
                "D8",
                "GND",
            ],
        ),
        KicadSymbol(
            name="DiceLoop:LED_Bar_74HC595",
            ref_prefix="U",
            value="LED_Bar_74HC595",
            pins=[
                "D2",
                "D3",
                "D4",
                "3V3",
                "GND",
            ],
        ),
    ]
    if include_oled:
        symbols.append(
            KicadSymbol(
                name="DiceLoop:SSD1306_OLED",
                ref_prefix="U",
                value="SSD1306_OLED",
                pins=[
                    "SDA",
                    "SCL",
                    "D5",
                    "D6",
                    "3V3",
                    "GND",
                ],
            )
        )
    return symbols


def build_kicad_instances(include_oled: bool) -> List[KicadInstance]:
    instances = [
        KicadInstance("DiceLoop:Teensy_4_0", "U1", 50.0, 120.0),
        KicadInstance("DiceLoop:Audio_Shield", "U2", 160.0, 40.0),
        KicadInstance("DiceLoop:Control_Pots", "J1", 160.0, 120.0),
        KicadInstance("DiceLoop:Chaos_Buttons", "SW1", 160.0, 190.0),
        KicadInstance("DiceLoop:LED_Bar_74HC595", "U3", 160.0, 245.0),
    ]
    if include_oled:
        instances.append(KicadInstance("DiceLoop:SSD1306_OLED", "U4", 160.0, 310.0))
    return instances


def render_kicad_wires_and_labels(
    instances: List[KicadInstance], symbols_by_name: Dict[str, KicadSymbol]
) -> str:
    lines: List[str] = []
    for instance in instances:
        symbol = symbols_by_name[instance.symbol_name]
        for pin_name, y_offset in kicad_pin_offsets(symbol.pins):
            pin_x = instance.x - 5.08
            pin_y = instance.y + y_offset
            label_x = pin_x - 2.54
            label_y = pin_y
            lines.extend(
                [
                    "\t(wire",
                    "\t\t(pts",
                    f"\t\t\t(xy {kfmt(pin_x)} {kfmt(pin_y)}) (xy {kfmt(label_x)} {kfmt(label_y)})",
                    "\t\t)",
                    "\t\t(stroke",
                    "\t\t\t(width 0)",
                    "\t\t\t(type solid)",
                    "\t\t)",
                    f'\t\t(uuid "{kicad_uuid()}")',
                    "\t)",
                    f'\t(label "{pin_name}"',
                    f"\t\t(at {kfmt(label_x)} {kfmt(label_y)} 0)",
                    "\t\t(effects",
                    "\t\t\t(font",
                    "\t\t\t\t(size 1.27 1.27)",
                    "\t\t\t)",
                    "\t\t\t(justify left bottom)",
                    "\t\t)",
                    f'\t\t(uuid "{kicad_uuid()}")',
                    "\t)",
                ]
            )
    return "\n".join(lines)


def render_kicad_instance(
    instance: KicadInstance,
    symbol: KicadSymbol,
    root_uuid: str,
    project_name: str,
) -> str:
    pin_count = len(symbol.pins)
    height = (pin_count - 1) * 2.54
    ref_y = instance.y + height / 2 + 5.08
    val_y = instance.y - height / 2 - 5.08
    lines = [
        "\t(symbol",
        f'\t\t(lib_id "{symbol.name}")',
        f"\t\t(at {kfmt(instance.x)} {kfmt(instance.y)} 0)",
        "\t\t(unit 1)",
        "\t\t(exclude_from_sim no)",
        "\t\t(in_bom yes)",
        "\t\t(on_board yes)",
        "\t\t(dnp no)",
        f'\t\t(uuid "{kicad_uuid()}")',
        f'\t\t(property "Reference" "{instance.reference}"',
        f"\t\t\t(at {kfmt(instance.x)} {kfmt(ref_y)} 0)",
        "\t\t\t(effects",
        "\t\t\t\t(font",
        "\t\t\t\t\t(size 1.27 1.27)",
        "\t\t\t\t)",
        "\t\t\t)",
        "\t\t)",
        f'\t\t(property "Value" "{symbol.value}"',
        f"\t\t\t(at {kfmt(instance.x)} {kfmt(val_y)} 0)",
        "\t\t\t(effects",
        "\t\t\t\t(font",
        "\t\t\t\t\t(size 1.27 1.27)",
        "\t\t\t\t)",
        "\t\t\t)",
        "\t\t)",
        '\t\t(property "Footprint" ""',
        f"\t\t\t(at {kfmt(instance.x)} {kfmt(instance.y)} 0)",
        "\t\t\t(effects",
        "\t\t\t\t(font",
        "\t\t\t\t\t(size 1.27 1.27)",
        "\t\t\t\t)",
        "\t\t\t\t(hide yes)",
        "\t\t\t)",
        "\t\t)",
        '\t\t(property "Datasheet" ""',
        f"\t\t\t(at {kfmt(instance.x)} {kfmt(instance.y)} 0)",
        "\t\t\t(effects",
        "\t\t\t\t(font",
        "\t\t\t\t\t(size 1.27 1.27)",
        "\t\t\t\t)",
        "\t\t\t\t(hide yes)",
        "\t\t\t)",
        "\t\t)",
        '\t\t(property "Description" ""',
        f"\t\t\t(at {kfmt(instance.x)} {kfmt(instance.y)} 0)",
        "\t\t\t(effects",
        "\t\t\t\t(font",
        "\t\t\t\t\t(size 1.27 1.27)",
        "\t\t\t\t)",
        "\t\t\t\t(hide yes)",
        "\t\t\t)",
        "\t\t)",
    ]
    for idx in range(1, pin_count + 1):
        lines.extend(
            [
                f'\t\t(pin "{idx}"',
                f'\t\t\t(uuid "{kicad_uuid()}")',
                "\t\t)",
            ]
        )
    lines.extend(
        [
            "\t\t(instances",
            f'\t\t\t(project "{project_name}"',
            f'\t\t\t\t(path "/{root_uuid}"',
            f'\t\t\t\t\t(reference "{instance.reference}")',
            "\t\t\t\t\t(unit 1)",
            "\t\t\t\t)",
            "\t\t\t)",
            "\t\t)",
            "\t)",
        ]
    )
    return "\n".join(lines)


def render_kicad(include_oled: bool) -> str:
    root_uuid = kicad_uuid()
    project_name = "dice-loop-schematic"
    symbols = build_kicad_symbols(include_oled)
    instances = build_kicad_instances(include_oled)
    symbols_by_name = {symbol.name: symbol for symbol in symbols}

    symbol_defs = "\n".join(render_kicad_symbol(symbol) for symbol in symbols)
    wires_labels = render_kicad_wires_and_labels(instances, symbols_by_name)
    instance_blocks = "\n".join(
        render_kicad_instance(instance, symbols_by_name[instance.symbol_name], root_uuid, project_name)
        for instance in instances
    )

    return "\n".join(
        [
            "(kicad_sch",
            "\t(version 20250114)",
            '\t(generator "eeschema")',
            '\t(generator_version "9.0")',
            f'\t(uuid "{root_uuid}")',
            '\t(paper "A4")',
            "\t(title_block",
            '\t\t(title "DiceLoop hardware schematic")',
            '\t\t(rev "1")',
            "\t)",
            "\t(lib_symbols",
            symbol_defs,
            "\t)",
            wires_labels,
            instance_blocks,
            "\t(sheet_instances",
            '\t\t(path "/"',
            '\t\t\t(page "1")',
            "\t\t)",
            "\t)",
            "\t(embedded_fonts no)",
            ")",
        ]
    )


def write_file(path: Path, content: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(content, encoding="utf-8")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Generate a logical DiceLoop hardware schematic (SVG + Markdown)."
    )
    parser.add_argument(
        "--output-svg",
        default=str(DEFAULT_SVG),
        help=f"SVG output path (default: {DEFAULT_SVG})",
    )
    parser.add_argument(
        "--output-md",
        default=str(DEFAULT_MD),
        help=f"Markdown output path (default: {DEFAULT_MD})",
    )
    parser.add_argument(
        "--output-kicad",
        default=str(DEFAULT_KICAD),
        help=f"KiCad schematic output path (default: {DEFAULT_KICAD})",
    )
    parser.add_argument(
        "--no-oled",
        action="store_true",
        help="Omit the optional OLED block from the schematic.",
    )
    return parser.parse_args()


def main() -> None:
    args = parse_args()
    include_oled = not args.no_oled

    blocks = build_blocks(include_oled=include_oled)
    wires = build_wires(blocks, include_oled=include_oled)
    svg = render_svg(blocks, wires)
    md = render_md(include_oled=include_oled)
    kicad = render_kicad(include_oled=include_oled)

    write_file(Path(args.output_svg), svg)
    write_file(Path(args.output_md), md)
    write_file(Path(args.output_kicad), kicad)


if __name__ == "__main__":
    main()
