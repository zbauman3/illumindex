import { main } from "../../../main";

export async function GET() {
  const commands = await main();
  return Response.json(commands);
}
