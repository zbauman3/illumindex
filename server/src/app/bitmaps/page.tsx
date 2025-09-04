"use client"

import {
  MouseEventHandler,
  RefObject,
  useCallback,
  useRef,
  useState,
} from "react"
import { BitmapComponent } from "@/components/Bitmap"
import { type Bitmap, createBitmap, mergeBitmaps } from "@/lib"

export const dynamic = "error"

const getTargetIndex = ({ target }: { target?: EventTarget }) => {
  if (!(target instanceof HTMLDivElement)) {
    return -1
  }

  const index = parseInt(target.getAttribute("data-index") || "")
  if (isNaN(index)) {
    return -1
  }

  return index
}

const bitmapToTextfieldData = (bitmap: Bitmap) => {
  return JSON.stringify(bitmap.data)
}

const DrawingBitmap = ({
  bitmapRef,
}: {
  bitmapRef: RefObject<Bitmap | undefined>
}) => {
  const [bitmap, setBitmap] = useState<Bitmap>(createBitmap(64, 64))
  bitmapRef.current = bitmap
  const [red, setRed] = useState<string>("255")
  const [green, setGreen] = useState<string>("255")
  const [blue, setBlue] = useState<string>("255")
  const [display, setDisplay] = useState<boolean>(false)
  const [newWidth, setNewWidth] = useState<string>(bitmap.size.width.toString())
  const [newHeight, setNewHeight] = useState<string>(
    bitmap.size.height.toString()
  )
  const [clearMode, setClearMode] = useState<boolean>(false)
  const [textAreaValue, setTextAreaValue] = useState<string>("")
  const mouseDownRef = useRef<boolean>(false)
  const [chooseColor, setChooseColor] = useState<boolean>(false)

  const activatePixel = useCallback(
    (index: number) => {
      setBitmap((prev) => {
        const newBitmap = { ...prev }
        if (clearMode) {
          newBitmap.data.red[index] = 0
          newBitmap.data.green[index] = 0
          newBitmap.data.blue[index] = 0
        } else {
          newBitmap.data.red[index] = parseInt(red || "0")
          newBitmap.data.green[index] = parseInt(green || "0")
          newBitmap.data.blue[index] = parseInt(blue || "0")
        }
        return newBitmap
      })
    },
    [red, green, blue, clearMode]
  )

  const onMouseDownHandler = useCallback<MouseEventHandler>(
    ({ target }) => {
      const index = getTargetIndex({ target })
      if (index === -1) {
        return
      }

      if (chooseColor) {
        setChooseColor(false)
        if (!(target instanceof HTMLDivElement)) {
          return
        }
        const color = window
          .getComputedStyle(target)
          .backgroundColor.match(/\d+/g)
        if (!color || color.length !== 3) {
          return
        }
        setRed(color[0])
        setGreen(color[1])
        setBlue(color[2])
        return
      }

      mouseDownRef.current = true
      activatePixel(index)
    },
    [activatePixel, chooseColor]
  )

  const onMouseUpHandler = useCallback<MouseEventHandler>(() => {
    mouseDownRef.current = false
  }, [])

  const onMouseEnterHandler = useCallback<MouseEventHandler>(
    ({ target }) => {
      if (!mouseDownRef.current) {
        return
      }

      const index = getTargetIndex({ target })
      if (index > -1) {
        activatePixel(index)
      }
    },
    [activatePixel]
  )

  const clearBitmap = useCallback(() => {
    setBitmap(createBitmap(bitmap.size.width, bitmap.size.height))
  }, [bitmap.size.width, bitmap.size.height])

  const setNewWidthAndHeight = useCallback((width: number, height: number) => {
    if (isNaN(width) || isNaN(height) || width < 1 || height < 1) {
      return
    }
    setBitmap(createBitmap(width, height))
  }, [])

  const hydrateFromTextarea = useCallback(() => {
    try {
      const data = JSON.parse(textAreaValue)
      if (
        !data ||
        typeof data !== "object" ||
        !("red" in data) ||
        !("green" in data) ||
        !("blue" in data) ||
        !Array.isArray(data.red) ||
        !Array.isArray(data.green) ||
        !Array.isArray(data.blue) ||
        data.red.length !== bitmap.size.width * bitmap.size.height ||
        data.green.length !== bitmap.size.width * bitmap.size.height ||
        data.blue.length !== bitmap.size.width * bitmap.size.height
      ) {
        alert(
          "The value must be `{ red: [], green: [], blue: [] }` where each array is the correct length."
        )
        return
      }
      setBitmap({
        size: { width: bitmap.size.width, height: bitmap.size.height },
        data,
      })
      setDisplay(false)
      setTextAreaValue("")
    } catch (e) {
      alert(e)
    }
  }, [bitmap.size.width, bitmap.size.height, textAreaValue])

  const saveToClipboard = useCallback(async () => {
    try {
      await navigator.clipboard.writeText(bitmapToTextfieldData(bitmap))
    } catch (error) {
      if (error instanceof Error) {
        alert(error.message)
      } else {
        alert("Error with clipboard. See the console for details.")
        console.error(error)
      }
    }
  }, [bitmap])

  return (
    <div>
      <BitmapComponent
        bitmap={bitmap}
        onMouseDown={onMouseDownHandler}
        onMouseUp={onMouseUpHandler}
        onMouseLeave={onMouseUpHandler}
        onMouseOverCapture={onMouseEnterHandler}
      />
      <div style={{ paddingTop: 10 }}>
        <div
          style={{ display: "flex", flexDirection: "row", flexWrap: "nowrap" }}
        >
          <div>
            R:{" "}
            <input
              type="range"
              min="0"
              max="255"
              value={red}
              onChange={(e) => setRed(e.target.value)}
            />{" "}
            <input
              type="text"
              value={red}
              onChange={(e) => setRed(e.target.value)}
            />{" "}
            <br />
            G:{" "}
            <input
              type="range"
              min="0"
              max="255"
              value={green}
              onChange={(e) => setGreen(e.target.value)}
            />{" "}
            <input
              type="text"
              value={green}
              onChange={(e) => setGreen(e.target.value)}
            />{" "}
            <br />
            B:{" "}
            <input
              type="range"
              min="0"
              max="255"
              value={blue}
              onChange={(e) => setBlue(e.target.value)}
            />{" "}
            <input
              type="text"
              value={blue}
              onChange={(e) => setBlue(e.target.value)}
            />{" "}
            <br />
            <button onClick={clearBitmap}>Clear Sheet</button> <br /> <br />
          </div>
          <div style={{ paddingLeft: 20 }}>
            <div
              style={{
                height: 40,
                width: 40,
                backgroundColor: `rgb(${red}, ${green}, ${blue})`,
                border: "1px solid gray",
              }}
            />
            <label>
              Eraser:{" "}
              <input
                type="checkbox"
                checked={clearMode}
                onChange={() => setClearMode(!clearMode)}
              />
            </label>
            <br />
            <label>
              Match:{" "}
              <input
                type="checkbox"
                checked={chooseColor}
                onChange={() => setChooseColor(!chooseColor)}
              />
            </label>
          </div>
        </div>
        W:{" "}
        <input
          type="number"
          value={newWidth}
          onChange={(e) => setNewWidth(e.target.value)}
        />{" "}
        <br />
        H:{" "}
        <input
          type="number"
          value={newHeight}
          onChange={(e) => setNewHeight(e.target.value)}
        />{" "}
        <br />
        <button
          onClick={() =>
            setNewWidthAndHeight(parseInt(newWidth), parseInt(newHeight))
          }
        >
          Clear & Set Size
        </button>{" "}
        <br /> <br />
        <button
          onClick={() => {
            setTextAreaValue(bitmapToTextfieldData(bitmap))
            setDisplay(!display)
          }}
        >
          Toggle JSON
        </button>
        <button onClick={saveToClipboard}>Copy Value</button>
      </div>
      {display && (
        <>
          <button onClick={hydrateFromTextarea}>Save JSON To Matrix</button>
          <div style={{ whiteSpace: "pre-wrap", wordBreak: "break-all" }}>
            Width: {bitmap.size.width} Height: {bitmap.size.height} Data:
            <br />
            <textarea
              value={textAreaValue}
              onChange={(e) => setTextAreaValue(e.target.value)}
            />
          </div>
        </>
      )}
    </div>
  )
}

const SavedBitmap = ({
  drawingBitmapRef,
}: {
  drawingBitmapRef: RefObject<Bitmap | undefined>
}) => {
  const [currentBitmap, setCurrentBitmap] = useState<Bitmap | undefined>(
    undefined
  )
  const [prevBitmaps, setPrevBitmaps] = useState<Bitmap[]>([])
  const [merge, setMerge] = useState<boolean>(true)
  const [display, setDisplay] = useState<boolean>(false)

  const copyBitmap = useCallback<MouseEventHandler>(() => {
    if (!drawingBitmapRef.current) {
      return
    }
    const bitmap = {
      ...drawingBitmapRef.current,
      data: {
        red: [...drawingBitmapRef.current.data.red],
        green: [...drawingBitmapRef.current.data.green],
        blue: [...drawingBitmapRef.current.data.blue],
      },
    }

    setPrevBitmaps((c) => (currentBitmap ? [...c, currentBitmap] : c))
    if (merge) {
      if (!currentBitmap) {
        setCurrentBitmap(
          mergeBitmaps({
            base: createBitmap(bitmap.size.width, bitmap.size.height),
            overlays: [bitmap],
            offsetX: 0,
            offsetY: 0,
          })
        )
      } else {
        setCurrentBitmap(
          mergeBitmaps({
            base: currentBitmap,
            overlays: [bitmap],
            offsetX: 0,
            offsetY: 0,
          })
        )
      }
    } else {
      setCurrentBitmap(bitmap)
    }
  }, [drawingBitmapRef, merge, currentBitmap])

  const hasPrevious = prevBitmaps.length > 0

  const undoLastChange = useCallback(() => {
    const prevBitmap = prevBitmaps.pop()
    if (!prevBitmap) {
      return
    }
    setCurrentBitmap(prevBitmap)
    setPrevBitmaps([...prevBitmaps])
  }, [prevBitmaps])

  const clearBitmap = useCallback(() => {
    if (!confirm("Clear the current bitmap and history?")) {
      return
    }
    setPrevBitmaps([])
    setCurrentBitmap(undefined)
  }, [])

  return (
    <div>
      {currentBitmap && <BitmapComponent bitmap={currentBitmap} />}
      <div style={{ paddingTop: 10 }}>
        <button disabled={!currentBitmap} onClick={clearBitmap}>
          Clear
        </button>{" "}
        <br />
        <button disabled={!hasPrevious} onClick={undoLastChange}>
          Revert
        </button>{" "}
        <br />
        <button onClick={copyBitmap}>Apply Bitmap</button> <br />
        <label>
          Merge:{" "}
          <input
            type="checkbox"
            checked={merge}
            onChange={() => setMerge(!merge)}
          />
        </label>{" "}
        <br /> <br />
        <button disabled={!currentBitmap} onClick={() => setDisplay(!display)}>
          Toggle JSON
        </button>
      </div>
      {currentBitmap && display && (
        <div style={{ whiteSpace: "pre-wrap", wordBreak: "break-all" }}>
          Width: {currentBitmap.size.width} Height: {currentBitmap.size.height}{" "}
          Data:
          <br />
          <textarea
            value={bitmapToTextfieldData(currentBitmap)}
            onChange={() => {}}
          />
        </div>
      )}
    </div>
  )
}

export default function Page() {
  const drawingBitmapRef = useRef<Bitmap | undefined>(undefined)

  return (
    <div
      style={{
        padding: `10px 10px 200px 10px`,
        fontFamily: "monospace",
        display: "flex",
        flexDirection: "row",
        flexWrap: "nowrap",
        justifyContent: "flex-start",
        alignItems: "flex-start",
        gap: 10,
      }}
    >
      <DrawingBitmap bitmapRef={drawingBitmapRef} />
      <SavedBitmap drawingBitmapRef={drawingBitmapRef} />
    </div>
  )
}
