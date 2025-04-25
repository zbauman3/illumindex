// import { NextRequest } from 'next/server';
// import z from "zod";

import { defaultCommandConfig } from "../../../lib/storage";
// import { commandsArray, type CommandsArray } from "../../../lib/commands";
import { main } from "../../../main";

// const postBodySchema = z.object({
//   commands: z.string()
// })

export async function GET() {
  // const config = await getCommandsConfig();

  // if (config.dataSource === 'storage') {
  //   const commands = await getCommands();
  //   return Response.json(commands);
  // }

  const commands = await main(defaultCommandConfig);
  return Response.json(commands);
}

// export async function POST(request: NextRequest) {
//   let parsedCommands: CommandsArray;

//   try {
//     const body = await request.json();
//     const parsedBody = postBodySchema.parse(body);
//     parsedCommands = commandsArray.parse(JSON.parse(parsedBody.commands))
//   } catch (e) {
//     console.error('POST error', e);
//     return new Response(null, { status: 400 });
//   }

//   await setCommands(parsedCommands);

//   return Response.json(parsedCommands);
// }