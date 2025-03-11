import { Tagged } from "./types";

export type Color565 = Tagged<number, '565-color'>

export const ensure_uint8_t = (v: number) => Math.max(Math.min(v, 255), 0);
export const ensure_uint16_t = (v: number): Color565 => Math.max(Math.min(v, 65535), 0) as Color565;
export const ensure565Color = ensure_uint16_t;

export const rgbTo565 = (red: number, green: number, blue: number): Color565 => (
  ((0b11111000 & ensure_uint8_t(red)) << 8) |
  ((0b11111100 & ensure_uint8_t(green)) << 3) |
  ((0b11111000 & ensure_uint8_t(blue)) >> 3)
) as Color565;

export const dec2bin = (dec: number) => { return (dec >>> 0).toString(2); }