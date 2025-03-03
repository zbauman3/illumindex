import { NextRequest } from "next/server";

export const config = {
  matcher: '/(api/commands|commands)',
};

const password = process.env.SITE_PASSWORD;

export default function middleware(request: NextRequest) {
  const token = request.nextUrl.searchParams.get('token');
  if (!password || token !== password) {
    return new Response(null, { status: 404 });
  }
}