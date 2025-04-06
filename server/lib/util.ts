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
  const red = (parsedColor & 0b1111100000000000) >> 8;
  const green = (parsedColor & 0b0000011111100000) >> 3;
  const blue = (parsedColor & 0b0000000000011111) << 3;
  return {
    red: ensure_uint8_t(red),
    green: ensure_uint8_t(green),
    blue: ensure_uint8_t(blue),
  }
}

export const dec2bin = (dec: number) => { return (dec >>> 0).toString(2); }