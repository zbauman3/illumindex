import { NextRequest } from "next/server";
import { main } from "../../../main";
import { createHash } from 'node:crypto'

export async function GET(request: NextRequest) {
  const commands = await main();

  // create a ETag of that data
  const hash = createHash('md5');
  hash.update(JSON.stringify(commands));
  const etag = hash.digest('hex');

  const headers = new Headers();
  headers.set('Cache-Control', 'no-cache');
  headers.set('ETag', etag);

  // check if the client already has the data
  const incomingEtag = request.headers.get('If-None-Match');
  if (incomingEtag && incomingEtag === etag) {
    return new Response(null, {
      status: 304,
      headers: headers
    });
  }

  return Response.json(commands, {
    headers: headers
  });
}
