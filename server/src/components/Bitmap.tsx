import { ComponentProps } from "react"
import { type Bitmap, type ColorRGB } from "@/lib"

const bitmapSpace = 1
const dotSize = 16

const BitmapDot = ({ color, index }: { index: number; color: ColorRGB }) => {
  const { red, blue, green } = color
  return (
    <div
      style={{
        flexGrow: 0,
        flexShrink: 0,
        width: dotSize + bitmapSpace * 2,
        height: dotSize + bitmapSpace * 2,
      }}
    >
      <div
        data-index={index}
        style={{
          display: "inline-block",
          width: dotSize,
          height: dotSize,
          borderRadius: "100%",
          backgroundColor: `rgb(${red}, ${green}, ${blue})`,
          margin: bitmapSpace,
        }}
      />
    </div>
  )
}

const BitmapRow = ({ rowNum, row }: { rowNum: number; row: ColorRGB[] }) => {
  const indexBase = rowNum * row.length
  return (
    <div
      style={{
        flexGrow: 0,
        flexShrink: 0,
        display: "inline-flex",
        flexDirection: "row",
        flexWrap: "nowrap",
        width: "auto",
      }}
    >
      {row.map((val, i) => (
        <BitmapDot key={`${rowNum}-${i}`} index={indexBase + i} color={val} />
      ))}
    </div>
  )
}

export const BitmapComponent = ({
  bitmap,
  ...rest
}: { bitmap: Bitmap } & ComponentProps<"div">) => {
  const matrix = bitmap.data.red.reduce((rows, val, index) => {
    if (index % bitmap.size.width == 0) {
      rows.push([
        {
          red: val,
          green: bitmap.data.green[index],
          blue: bitmap.data.blue[index],
        },
      ])
    } else {
      rows[rows.length - 1].push({
        red: val,
        green: bitmap.data.green[index],
        blue: bitmap.data.blue[index],
      })
    }
    return rows
  }, [] as ColorRGB[][])

  return (
    <div
      {...rest}
      style={{
        flexGrow: 0,
        flexShrink: 0,
        display: "flex",
        flexDirection: "column",
        flexWrap: "nowrap",
        padding: bitmapSpace * 2,
        backgroundColor: "#111111",
        width: "auto",
      }}
    >
      {matrix.map((row, i) => (
        <BitmapRow key={i} row={row} rowNum={i} />
      ))}
    </div>
  )
}
