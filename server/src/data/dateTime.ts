import {
  Bitmap,
  createBitmap,
  drawCommands,
  fontSizeDetailsMap,
  rgbTo565,
  Size,
} from "@/lib"
import { SCREEN } from "./constants"

const getTimeInZone = (timezone?: string) => {
  const currentTZ = process.env.TZ
  process.env.TZ = timezone || currentTZ
  const date = new Date()
  process.env.TZ = currentTZ
  return date
}

export const generateDateTimeBitmap = ({
  timezone,
}: {
  timezone?: string
}): { bitmap: Bitmap } & Size => {
  const height = fontSizeDetailsMap.sm.height + fontSizeDetailsMap.lg.height
  const width = SCREEN.width
  const date = getTimeInZone(timezone)

  const timeOptions: Intl.DateTimeFormatOptions = {
    hour: "numeric",
    minute: "2-digit",
    hour12: true,
  }

  const dateOptions: Intl.DateTimeFormatOptions = {
    weekday: "short",
    month: "short",
    day: "numeric",
  }

  return {
    height,
    width,
    bitmap: drawCommands({
      bitmap: createBitmap(width, height),
      commands: [
        {
          type: "string",
          value: date.toLocaleString("en-US", timeOptions),
          position: { x: 0, y: 0 },
          fontSize: "lg",
          color: rgbTo565(255, 255, 255),
        },
        {
          type: "line-feed",
        },
        {
          type: "string",
          value: date.toLocaleString("en-US", dateOptions),
          fontSize: "sm",
        },
      ],
    }),
  }
}
