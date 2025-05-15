import { useMemo } from "react";
import { BitmapComponent } from "../components/Bitmap";
import { createEmptyBitmap } from "../lib/bitmaps";
import { main } from "../main";
import { drawCommands } from "../lib/drawCommands";
import { type CommandsArray } from "@/lib/commands";

const RenderCommands = ({ commands }: { commands: CommandsArray }) => {
  const bitmap = useMemo(() => {
    const newBitmap = createEmptyBitmap(64, 64);
    const updateBitmap = drawCommands({ bitmap: newBitmap, commands });
    return updateBitmap;
  }, [commands]);

  return (
    <div
      style={{
        display: 'flex',
        flexDirection: 'row',
        flexWrap: 'nowrap',
        justifyContent: 'center',
        alignItems: 'center',
      }}
    >
      <BitmapComponent bitmap={bitmap} />
    </div>
  )
}

const Page = async () => {
  const commands = await main();

  return <RenderCommands commands={commands} />
}

export default Page