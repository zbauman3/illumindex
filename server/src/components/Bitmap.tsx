import { ComponentProps } from "react"
import { type Bitmap, uint16_tTo255RGB } from "@/lib"

const bitmapSpace = 1
const dotSize = 16

const BitmapDot = ({ color, index }: { index: number; color: number }) => {
  const { red, blue, green } = uint16_tTo255RGB(color)
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

const BitmapRow = ({ rowNum, row }: { rowNum: number; row: number[] }) => {
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
  const matrix = bitmap.data.reduce((rows, val, index) => {
    if (index % bitmap.size.width == 0) {
      rows.push([val])
    } else {
      rows[rows.length - 1].push(val)
    }
    return rows
  }, [] as number[][])

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
