
// export type Point = {
//   x: number,
//   y: number,
// };

// // export type DisplayBuffer = string[];

// const width = 64;
// const height = 32;

// export class DisplayBuffer {
//   buffer: string[];

//   constructor() {
//     this.buffer = new Array(width * height).fill('');
//   }

//   canSetPoint(point: Point) {

//   }
// }



// const setMatrixValue = (point: Point, value: string) => {
//   if (point.x >= width || point.y >= height) {
//     console.log('Cannot set point', point);
//     return;
//   }

//   matrixData[(point.y * width) + point.x] = value;
// }