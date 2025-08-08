import type { NextConfig } from "next"

if (typeof process.env.WEATHER_TZ !== "string") {
  throw new Error("No weather Timezone set")
}

process.env.TZ = process.env.WEATHER_TZ

const nextConfig: NextConfig = {
  env: {
    TZ: process.env.WEATHER_TZ,
  },
  /* config options here */
}

export default nextConfig
