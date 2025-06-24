import { defaultState } from "@/data/remoteState"

export async function GET() {
  return Response.json(defaultState)
}
