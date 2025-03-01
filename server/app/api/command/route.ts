import { main } from "../../../main";

export async function GET() {
  return Response.json(main());
}