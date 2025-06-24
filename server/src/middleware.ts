import { NextRequest } from "next/server"

export const config = {
  matcher:
    "/((?!_next/static|_next/image|favicon.ico|sitemap.xml|robots.txt|bitmaps).*)",
}

const password = process.env.SITE_PASSWORD
const isLocalDev = process.env.VERCEL !== "1"

export default function middleware(request: NextRequest) {
  // Allow local development without password
  if (isLocalDev) {
    return
  }

  let token: string | null | undefined = request.headers
    .get("authorization")
    ?.split(/Bearer:\s*/)[1]
  if (!token) {
    token = request.nextUrl.searchParams.get("token")
  }

  if (!password || token !== password) {
    return new Response(null, { status: 404 })
  }
}
