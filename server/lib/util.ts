export const ensure_uint8_t = (v: number) => Math.max(Math.min(v, 255), 0);

export const rgbTo565 = (red: number, green: number, blue: number) => ((0b11111000 & ensure_uint8_t(red)) << 8) | ((0b11111100 & ensure_uint8_t(green)) << 3) | ((0b11111000 & ensure_uint8_t(blue)) >> 3)

export const dec2bin = (dec: number) => { return (dec >>> 0).toString(2); }