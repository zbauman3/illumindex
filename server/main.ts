import { createCommand, type AllCommands } from "./lib/commands";
import { rgbTo565 } from "./lib/util";

export const main = (): AllCommands[] => {
  return [
    createCommand({
      type: 'set-state',
      color: rgbTo565(255, 0, 255),
      fontSize: 'md',
      position: {
        x: 10,
        y: 10
      }
    }),
    createCommand({
      type: 'string',
      value: 'Server',
    }),
    createCommand({
      type: 'set-state',
      fontSize: 'sm',
    }),
    createCommand({
      type: 'string',
      value: 'test',
    }),
    createCommand({
      type: 'line-feed'
    }),
    createCommand({
      type: 'string',
      value: 'nl & bm:',
    }),
    createCommand({
      type: 'bitmap',
      data: [
        rgbTo565(255, 0, 0), rgbTo565(0, 255, 0), rgbTo565(0, 0, 255),
        rgbTo565(0, 0, 255), rgbTo565(0, 255, 0), rgbTo565(255, 0, 0),
      ],
      size: { width: 3, height: 2 }
    }),
    createCommand({
      type: 'bitmap',
      data: [
        rgbTo565(255, 255, 255), rgbTo565(255, 255, 255), rgbTo565(255, 255, 255),
        rgbTo565(0, 0, 0), rgbTo565(0, 0, 0), rgbTo565(255, 255, 255),
        rgbTo565(0, 0, 0), rgbTo565(255, 255, 255), rgbTo565(0, 0, 0),
        rgbTo565(255, 255, 255), rgbTo565(0, 0, 0), rgbTo565(0, 0, 0),
        rgbTo565(255, 255, 255), rgbTo565(255, 255, 255), rgbTo565(255, 255, 255),
      ],
      position: { x: 0, y: 0 },
      size: { width: 3, height: 5 }
    }),
    createCommand({
      type: 'string',
      value: 'This is some long text to wrap and show the second half',
      fontSize: 'md',
      position: {
        x: 0,
        y: 32
      }
    }),
    createCommand({
      type: 'line',
      color: rgbTo565(0, 255, 0),
      position: { x: 0, y: 63 },
      to: { x: 63, y: 0 }
    })
  ];
}