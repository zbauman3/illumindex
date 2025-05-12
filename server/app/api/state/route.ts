import { defaultState } from "@/lib/state";

export async function GET() {
  return Response.json(defaultState);
}