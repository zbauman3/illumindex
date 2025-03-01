/* eslint-disable @typescript-eslint/no-explicit-any */

type Length<T extends any[]> =
  T extends { length: infer L } ? L : never;

type BuildTuple<L extends number, T extends number[] = []> =
  T extends { length: L } ? T : BuildTuple<L, [...T, number]>;

type Decrement<N extends number> = BuildTuple<N> extends [any, ...infer R] ? Length<R> : 0

type Add<A extends number, B extends number> =
  Length<[...BuildTuple<A>, ...BuildTuple<B>]>;

type Multiply<A extends number, B extends number, Acc = A> =
  B extends 1
  ? Acc
  : Acc extends number
  ? Multiply<A, Decrement<B>, Add<A, Acc>>
  : never

type CommandString = {
  type: 'string',
  value: string,
  color: number,
  size: 'sm' | 'md' | 'lg',
  position: {
    x: number,
    y: number
  }
}

type CommandLine = {
  type: 'line',
  color: number,
  from: {
    x: number,
    y: number
  },
  to: {
    x: number,
    y: number
  }
}

type CommandBitmap<W extends number, H extends number> = {
  type: 'bitmap',
  data: BuildTuple<Multiply<W, H>>,
  position: {
    x: number,
    y: number
  },
  size: {
    width: W,
    height: H
  }
}

function createCommand<W extends number, H extends number>(config: CommandBitmap<W, H>): CommandBitmap<W, H>;
function createCommand(config: CommandString): CommandString;
function createCommand(config: CommandLine): CommandLine;
function createCommand(config: CommandBitmap<number, number> | CommandString | CommandLine) {
  return config
}

export async function GET() {
  const bitmap = createCommand({
    type: 'bitmap',
    data: [
      0b1111100000000000, 0b0000011111100000, 0b0000000000011111,
      0b0000000000011111, 0b0000011111100000, 0b1111100000000000,
    ],
    position: { x: 20, y: 20 },
    size: { width: 3, height: 2 }
  })

  const string = createCommand({
    type: 'string',
    value: 'Server Test',
    size: 'md',
    color: 0b1111100000000000,
    position: { x: 0, y: 0 }
  })

  const line = createCommand({
    type: 'line',
    color: 0b0000011111100000,
    from: { x: 0, y: 0 },
    to: { x: 63, y: 31 }
  })

  return Response.json([string, line, bitmap]);
}