import { useMemo } from "react";

type Point = {
  x: number,
  y: number,
};

const width = 64;
const height = 32;
const matrixData: string[] = new Array(width * height).fill('');
const getRandomColor = () => '#' + (Math.random() * 0xFFFFFF << 0).toString(16).padStart(6, '0');

const setMatrixValue = (point: Point, value: string) => {
  if (point.x >= width || point.y >= height) {
    console.log('Cannot set point', point);
    return;
  }

  matrixData[(point.y * width) + point.x] = value;
}

const fastVertLine = (point: Point, to: number, color: string) => {
  const newPoint = { ...point };
  if (newPoint.y < to) {
    const tmp = to;
    to = newPoint.y;
    newPoint.y = tmp;
  }

  while (newPoint.y >= to) {
    setMatrixValue(newPoint, color);
    newPoint.y--;
  }
}

const fastHorizonLine = (point: Point, to: number, color: string) => {
  const newPoint = { ...point };
  if (newPoint.x < to) {
    const tmp = to;
    to = newPoint.x;
    newPoint.x = tmp;
  }

  while (newPoint.x >= to) {
    setMatrixValue(newPoint, color);
    newPoint.x--;
  }
}

const drawLine = (from: Point, to: Point, color: string) => {
  if (from.x === to.x) {
    fastVertLine(from, to.y, color);
    return;
  }

  if (from.y === to.y) {
    fastHorizonLine(from, to.x, color);
    return;
  }


  const m = (to.y - from.y) / (to.x - from.x);
  let floatY = from.y;

  if (from.x <= to.x) {
    while (from.x <= to.x) {
      setMatrixValue({ ...from, y: Math.round(floatY) }, color);
      from.x++;
      floatY += m;
    }
  } else {
    while (from.x >= to.x) {
      setMatrixValue({ ...from, y: Math.round(floatY) }, color);
      from.x--;
      floatY -= m;
    }
  }
}

drawLine({ x: 32, y: 16 }, { x: 32, y: 0 }, getRandomColor())
drawLine({ x: 32, y: 16 }, { x: 32, y: 31 }, getRandomColor())
drawLine({ x: 32, y: 16 }, { x: 63, y: 16 }, getRandomColor())
drawLine({ x: 32, y: 16 }, { x: 0, y: 16 }, getRandomColor())

drawLine({ x: 32, y: 16 }, { x: 63, y: 31 }, getRandomColor())
drawLine({ x: 32, y: 16 }, { x: 63, y: 24 }, getRandomColor())
drawLine({ x: 32, y: 16 }, { x: 48, y: 31 }, getRandomColor())

drawLine({ x: 32, y: 16 }, { x: 63, y: 0 }, getRandomColor())
drawLine({ x: 32, y: 16 }, { x: 63, y: 8 }, getRandomColor())
drawLine({ x: 32, y: 16 }, { x: 48, y: 0 }, getRandomColor())

drawLine({ x: 32, y: 16 }, { x: 0, y: 0 }, getRandomColor())
drawLine({ x: 32, y: 16 }, { x: 16, y: 0 }, getRandomColor())
drawLine({ x: 32, y: 16 }, { x: 0, y: 8 }, getRandomColor())

drawLine({ x: 32, y: 16 }, { x: 0, y: 31 }, getRandomColor())
drawLine({ x: 32, y: 16 }, { x: 0, y: 24 }, getRandomColor())
drawLine({ x: 32, y: 16 }, { x: 16, y: 31 }, getRandomColor())

const Col = ({ value }: { value: string }) => (
  <div style={{ backgroundColor: value ? value : 'lightgray', width: 20, height: 20, borderRadius: 20 }} />
)

const Row = ({ columns }: { columns: string[] }) => (
  <div style={{ width: '100%', display: 'flex', flexDirection: 'row', flexWrap: 'nowrap' }}>
    {columns.map((colVal, i) => <Col key={i} value={colVal} />)}
  </div>
)

const Page = () => {
  const rows = useMemo(() => {
    let rowI = 0;
    return matrixData.reduce<string[][]>((acc, val, buffI) => {
      if (buffI !== 0 && buffI % width == 0) {
        rowI++;
      }

      if (!acc[rowI]) {
        acc[rowI] = [];
      }

      acc[rowI].push(val);
      return acc;
    }, []);
  }, []);


  return <div style={{ display: 'flex', flexDirection: 'column', flexWrap: 'nowrap' }}>
    {rows.map((columns, rowI) => <Row key={rowI} columns={columns} />)}
  </div>
}

export default Page