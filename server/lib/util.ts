export type Color565 = number

export const ensure_uint8_t = (v: number) => Math.max(Math.min(v, 255), 0);
export const ensure_uint16_t = (v: number): Color565 => Math.max(Math.min(v, 65535), 0) as Color565;
export const ensure565Color = ensure_uint16_t;

export const rgbTo565 = (red: number, green: number, blue: number): Color565 => (
  ((0b11111000 & ensure_uint8_t(red)) << 8) |
  ((0b11111100 & ensure_uint8_t(green)) << 3) |
  ((0b11111000 & ensure_uint8_t(blue)) >> 3)
) as Color565;

export const uint16_tTo255RGB = (color: number) => {
  const parsedColor = ensure_uint16_t(color);
  const red = (parsedColor & 0xF800) >> 8;
  const green = (parsedColor & 0x07E0) >> 3;
  const blue = (parsedColor & 0x001F) << 3;
  // now scale up the values to 255
  return {
    red: red * 8,
    green: green * 4,
    blue: blue * 8,
  }
}

export const dec2bin = (dec: number) => { return (dec >>> 0).toString(2); }