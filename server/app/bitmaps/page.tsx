'use client';

import { MouseEventHandler, RefObject, useCallback, useRef, useState } from "react"
import { rgbTo565 } from "../../lib/util"
import { BitmapComponent } from "../../components/Bitmap";
import { type Bitmap, createEmptyBitmap, mergeBitmaps } from "../../lib/bitmaps";

const getTargetIndex = ({ target }: { target?: EventTarget }) => {
  if (!(target instanceof HTMLDivElement)) {
    return -1;
  }

  const index = parseInt(target.getAttribute('data-index') || '')
  if (isNaN(index)) {
    return -1;
  }

  return index
}

const DrawingBitmap = ({ bitmapRef }: { bitmapRef: RefObject<Bitmap | undefined> }) => {
  const [bitmap, setBitmap] = useState<Bitmap>(createEmptyBitmap(64, 64));
  bitmapRef.current = bitmap;

  const [red, setRed] = useState<number>(255);
  const [green, setGreen] = useState<number>(255);
  const [blue, setBlue] = useState<number>(255);
  const [display, setDisplay] = useState<boolean>(false);
  const [newWidth, setNewWidth] = useState<string>(bitmap.size.width.toString())
  const [newHeight, setNewHeight] = useState<string>(bitmap.size.height.toString())
  const [clearMode, setClearMode] = useState<boolean>(false);
  const [textAreaValue, setTextAreaValue] = useState<string>('');
  const mouseDownRef = useRef<boolean>(false);

  const activatePixel = useCallback((index: number) => {
    setBitmap((prev) => {
      const newBitmap = { ...prev }
      newBitmap.data[index] = clearMode ? 0 : rgbTo565(red, green, blue)
      return newBitmap
    })
  }, [red, green, blue, clearMode])

  const onMouseDownHandler = useCallback<MouseEventHandler>(({ target }) => {
    mouseDownRef.current = true;
    const index = getTargetIndex({ target });
    if (index > -1) {
      activatePixel(index)
    }
  }, [activatePixel]);

  const onMouseUpHandler = useCallback<MouseEventHandler>(() => {
    mouseDownRef.current = false;
  }, []);

  const onMouseEnterHandler = useCallback<MouseEventHandler>(({ target }) => {
    if (!mouseDownRef.current) {
      return
    }

    const index = getTargetIndex({ target });
    if (index > -1) {
      activatePixel(index)
    }
  }, [activatePixel]);

  const clearBitmap = useCallback(() => {
    setBitmap(createEmptyBitmap(bitmap.size.width, bitmap.size.height))
  }, [bitmap.size.width, bitmap.size.height])

  const setNewWidthAndHeight = useCallback((width: number, height: number) => {
    if (isNaN(width) || isNaN(height) || width < 1 || height < 1) {
      return
    }
    setBitmap(createEmptyBitmap(width, height))
  }, [])

  const hydrateFromTextarea = useCallback(() => {
    try {
      const data = JSON.parse(`[${textAreaValue}]`)
      if (!Array.isArray(data) || data.length !== bitmap.size.width * bitmap.size.height) {
        alert('The value must be an array of the correct length.')
        return;
      }
      setBitmap({ size: { width: bitmap.size.width, height: bitmap.size.height }, data })
    } catch (e) {
      alert(e);
    }
  }, [bitmap.size.width, bitmap.size.height, textAreaValue]);

  return (
    <div>
      <BitmapComponent
        bitmap={bitmap}
        onMouseDown={onMouseDownHandler}
        onMouseUp={onMouseUpHandler}
        onMouseOverCapture={onMouseEnterHandler}
      />
      <div style={{ paddingTop: 10 }}>
        <div style={{ display: 'flex', flexDirection: 'row', flexWrap: 'nowrap' }}>
          <div>
            R: <input type="range" min="0" max="255" value={red} onChange={(e) => setRed(parseInt(e.target.value))} /> <br />
            G: <input type="range" min="0" max="255" value={green} onChange={(e) => setGreen(parseInt(e.target.value))} /> <br />
            B: <input type="range" min="0" max="255" value={blue} onChange={(e) => setBlue(parseInt(e.target.value))} /> <br />
            <button onClick={clearBitmap}>Clear Sheet</button> <br /> <br />
          </div>
          <div style={{ paddingLeft: 20 }}>
            <div style={{ height: 40, width: 40, backgroundColor: `rgb(${red}, ${green}, ${blue})`, border: '1px solid gray' }} />
            <label>Eraser: <input type="checkbox" checked={clearMode} onChange={() => setClearMode(!clearMode)} /></label>
          </div>
        </div>
        W: <input type="number" value={newWidth} onChange={(e) => setNewWidth(e.target.value)} /> <br />
        H: <input type="number" value={newHeight} onChange={(e) => setNewHeight(e.target.value)} /> <br />
        <button onClick={() => setNewWidthAndHeight(parseInt(newWidth), parseInt(newHeight))}>Clear & Set Size</button> <br /> <br />
        <button onClick={() => { setTextAreaValue(JSON.stringify(bitmap.data).replace(/\[|\]|,$|/g, '')); setDisplay(!display) }}>Toggle JSON</button>
      </div>
      {display && (
        <>
          <button onClick={hydrateFromTextarea}>Save JSON To Matrix</button>
          <div style={{ whiteSpace: 'pre-wrap', wordBreak: 'break-all' }}>
            Width: {bitmap.size.width} Height: {bitmap.size.height} Data:<br />
            <textarea value={textAreaValue} onChange={(e) => setTextAreaValue(e.target.value)} />
          </div>
        </>
      )}
    </div>
  )
}

const SavedBitmap = ({ drawingBitmapRef }: { drawingBitmapRef: RefObject<Bitmap | undefined> }) => {
  const [currentBitmap, setCurrentBitmap] = useState<Bitmap | undefined>(undefined);
  const [prevBitmap, setPrevBitmap] = useState<Bitmap | undefined>(undefined);
  const [merge, setMerge] = useState<boolean>(true);
  const [display, setDisplay] = useState<boolean>(false);

  const copyBitmap = useCallback(() => {
    const bitmap = drawingBitmapRef.current;
    if (!bitmap) {
      return
    }

    setPrevBitmap(currentBitmap);
    if (merge) {
      if (!currentBitmap) {
        setCurrentBitmap(mergeBitmaps({ base: createEmptyBitmap(bitmap.size.width, bitmap.size.height), overlay: bitmap, offsetX: 0, offsetY: 0 }));
      } else {
        setCurrentBitmap(mergeBitmaps({ base: currentBitmap, overlay: bitmap, offsetX: 0, offsetY: 0 }));
      }
    } else {
      setCurrentBitmap(bitmap);
    }
  }, [drawingBitmapRef, merge, currentBitmap]);

  const undoLastChange = useCallback(() => {
    setCurrentBitmap(prevBitmap)
    setPrevBitmap(undefined);
  }, [prevBitmap])

  const clearBitmap = useCallback(() => {
    setPrevBitmap(currentBitmap)
    setCurrentBitmap(undefined)
  }, [currentBitmap])

  return (
    <div>
      {currentBitmap && (
        <BitmapComponent bitmap={currentBitmap} />
      )}
      <div style={{ paddingTop: 10 }}>
        <button disabled={!prevBitmap} onClick={undoLastChange}>Revert</button> <br />
        <button disabled={!currentBitmap} onClick={clearBitmap}>Clear</button> <br />
        <button onClick={copyBitmap}>Apply Bitmap</button> <br />
        <label>Merge: <input type="checkbox" checked={merge} onChange={() => setMerge(!merge)} /></label> <br /> <br />
        <button disabled={!currentBitmap} onClick={() => setDisplay(!display)}>Toggle JSON</button>
      </div>
      {currentBitmap && display && (
        <div style={{ whiteSpace: 'pre-wrap', wordBreak: 'break-all' }}>
          Width: {currentBitmap.size.width} Height: {currentBitmap.size.height} Data:<br />
          <textarea value={JSON.stringify(currentBitmap.data).replace(/\[|\]|,$|/g, '')} />
        </div>
      )}
    </div>
  )
}

export default function Page() {
  const drawingBitmapRef = useRef<Bitmap | undefined>(undefined);

  return (
    <div
      style={{
        padding: `10px 10px 200px 10px`,
        fontFamily: 'monospace',
        display: 'flex',
        flexDirection: 'row',
        flexWrap: 'nowrap',
        justifyContent: 'flex-start',
        alignItems: 'flex-start',
        gap: 10
      }}
    >
      <DrawingBitmap bitmapRef={drawingBitmapRef} />
      <SavedBitmap drawingBitmapRef={drawingBitmapRef} />
    </div>
  )
}