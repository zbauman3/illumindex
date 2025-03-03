import { NextRequest } from 'next/server';
import { createClient } from 'redis';

const redis = await createClient({
  url: process.env.REDIS_URL
})
  .on('error', err => console.log('Redis Client Error', err))
  .connect();

// import { main } from "../../../main";

export async function GET() {
  const value = await redis.get('commands');
  const commands = JSON.parse(value || '');
  return Response.json(commands);
}


export async function POST(request: NextRequest) {
  const body = await request.json();
  if (!body || typeof body !== 'object' || !('commands' in body) || typeof body.commands !== 'string') {
    return new Response(null, { status: 400 });
  }

  // Just to make sure it's valid, first
  await redis.set('commands', JSON.stringify(JSON.parse(body.commands)));

  return GET();
}