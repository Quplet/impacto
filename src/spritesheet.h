﻿#pragma once

#include "util.h"
#include <enum.h>

namespace Impacto {

struct SpriteSheet {
  SpriteSheet() {}
  SpriteSheet(float width, float height)
      : DesignWidth(width), DesignHeight(height) {}

  float DesignWidth;
  float DesignHeight;

  GLuint Texture = 0;
};

struct Sprite {
  Sprite() : BaseScale(1.0f) {}
  Sprite(SpriteSheet const& sheet, float x, float y, float width, float height,
         glm::vec2 baseScale = glm::vec2(1.0f))
      : Sheet(sheet), Bounds(x, y, width, height), BaseScale(baseScale) {}

  SpriteSheet Sheet;
  RectF Bounds;
  glm::vec2 BaseScale;
};

// TODO dataify and move out of here, this file is supposed to be graphics code
BETTER_ENUM(CharacterTypeFlags, uint8_t, Space = (1 << 0),
            WordStartingPunct = (1 << 1), WordEndingPunct = (1 << 2))

// TODO these are properties of the font, make into one whole-charset width
// table and dataify
uint8_t const WidthTable[] = {
    0x20, 0x16, 0x0E, 0x14, 0x15, 0x16, 0x16, 0x16, 0x15, 0x16, 0x16, 0x18,
    0x16, 0x17, 0x17, 0x15, 0x14, 0x18, 0x16, 0x08, 0x0E, 0x17, 0x13, 0x1A,
    0x16, 0x19, 0x15, 0x19, 0x16, 0x15, 0x15, 0x16, 0x17, 0x1E, 0x16, 0x15,
    0x15, 0x14, 0x13, 0x13, 0x14, 0x14, 0x0D, 0x14, 0x13, 0x08, 0x09, 0x13,
    0x08, 0x1A, 0x13, 0x15, 0x13, 0x14, 0x0D, 0x12, 0x0D, 0x13, 0x13, 0x1A,
    0x13, 0x13, 0x12, 0x11, 0x0F, 0x08, 0x0F, 0x08, 0x08, 0x14, 0x08, 0x08,
    0x1C, 0x17, 0x1E, 0x0F, 0x11, 0x19, 0x0C, 0x0D, 0x0D, 0x0C, 0x14, 0x13,
    0x15, 0x12, 0x12, 0x12, 0x10, 0x10, 0x14, 0x19, 0x0E, 0x08, 0x0C, 0x0C,
    0x14, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A,
    0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A,
    0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x19, 0x0F, 0x17, 0x18,
    0x1A, 0x17, 0x18, 0x17, 0x19, 0x18, 0x19, 0x16, 0x17, 0x17, 0x14, 0x14,
    0x18, 0x15, 0x08, 0x0E, 0x17, 0x13, 0x1A, 0x15, 0x19, 0x15, 0x19, 0x16,
    0x15, 0x15, 0x15, 0x18, 0x1E, 0x17, 0x17, 0x15, 0x14, 0x14, 0x13, 0x14,
    0x13, 0x0E, 0x14, 0x13, 0x09, 0x0C, 0x13, 0x08, 0x19, 0x13, 0x15, 0x14,
    0x14, 0x0D, 0x12, 0x0E, 0x13, 0x14, 0x1B, 0x15, 0x14, 0x13, 0x12, 0x12,
    0x08, 0x08, 0x09, 0x09, 0x15, 0x09, 0x0C, 0x0B, 0x08, 0x08, 0x0D, 0x0E,
    0x0B, 0x0C, 0x0C, 0x0C, 0x0B, 0x0C, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11,
    0x0C, 0x0D, 0x0D, 0x0E, 0x0B, 0x0C, 0x16, 0x17, 0x0B, 0x0C, 0x09, 0x1D,
    0x1D, 0x1D, 0x14, 0x1E, 0x1A, 0x1A, 0x17, 0x19, 0x1A, 0x1A, 0x1B, 0x19,
    0x19, 0x1B, 0x19, 0x18, 0x17, 0x19, 0x19, 0x19, 0x1A, 0x19, 0x16, 0x17,
    0x18, 0x1B, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E,
    0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E,
    0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E,
    0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E,
    0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E,
    0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E,
    0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E,
    0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E,
    0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E,
    0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E,
    0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E,
};

struct Font {
  Font() {}
  Font(float width, float height, uint8_t columns, uint8_t rows)
      : Sheet(width, height), Columns(columns), Rows(rows) {}

  SpriteSheet Sheet;
  uint8_t Columns;
  uint8_t Rows;

  float RowHeight() const { return Sheet.DesignHeight / (float)Rows; }
  float ColWidth() const { return Sheet.DesignWidth / (float)Columns; }

  Sprite Glyph(uint8_t row, uint8_t col) { return Glyph(row * Columns + col); }

  Sprite Glyph(uint16_t id) {
    uint8_t row = id / Columns;
    uint8_t col = id % Columns;
    float width = ColWidth();
    if (id < 0x15E) {
      width = (float)WidthTable[id] / 32.0f * RowHeight();
    }
    return Sprite(Sheet, col * ColWidth(), row * RowHeight(), width,
                  RowHeight());
  }

  // TODO again, dataify and move out of here
  uint8_t CharacterType(uint16_t glyphId) {
    uint8_t result = 0;
    if (glyphId == 0 || glyphId == 63) result |= CharacterTypeFlags::Space;

    // TODO check if these are even the right ones for R;NE

    // 、 。 ． ， ？ ！ 〜 ” ー ） 〕 ］ ｝ 〉 》 」 』 】☆ ★ ♪ 々 ぁ ぃ ぅ ぇ
    // ぉ っ ゃ ゅ ょ ァ ィ ゥ ェ ォ ッ ャ ュ ョ –
    if (glyphId == 0x00BE || glyphId == 0x00BF || glyphId == 0x00C1 ||
        glyphId == 0x00C0 || glyphId == 0x00C4 || glyphId == 0x00C5 ||
        glyphId == 0x00E4 || glyphId == 0x00CB || glyphId == 0x00E5 ||
        glyphId == 0x00CD || glyphId == 0x00CF || glyphId == 0x00D1 ||
        glyphId == 0x00D3 || glyphId == 0x00D5 || glyphId == 0x00D7 ||
        glyphId == 0x00D9 || glyphId == 0x00DB || glyphId == 0x00DD ||
        glyphId == 0x01A5 || glyphId == 0x01A6 || glyphId == 0x00E6 ||
        glyphId == 0x0187 || glyphId == 0x00E8 || glyphId == 0x00E9 ||
        glyphId == 0x00EA || glyphId == 0x00EB || glyphId == 0x00EC ||
        glyphId == 0x00ED || glyphId == 0x00EE || glyphId == 0x00EF ||
        glyphId == 0x00F0 || glyphId == 0x00F2 || glyphId == 0x00F3 ||
        glyphId == 0x00F4 || glyphId == 0x00F5 || glyphId == 0x00F6 ||
        glyphId == 0x00F7 || glyphId == 0x00F8 || glyphId == 0x00F9 ||
        glyphId == 0x00FA || glyphId == 0x0113)
      result |= CharacterTypeFlags::WordEndingPunct;

    // space(63) space(0) “ （ 〔 ［ ｛ 〈 《 「
    if (glyphId == 63 || glyphId == 0 || glyphId == 0x00CA ||
        glyphId == 0x00CC || glyphId == 0x00CE || glyphId == 0x00D0 ||
        glyphId == 0x00D2 || glyphId == 0x00D4 || glyphId == 0x00D6 ||
        glyphId == 0x00D8 || glyphId == 0x00DA || glyphId == 0x00DC)
      result |= CharacterTypeFlags::WordStartingPunct;

    return result;
  }
};

}  // namespace Impacto