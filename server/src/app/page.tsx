import { AnimatedBitmap } from "@/components/AnimatedBitmap"
import { main } from "@/main"
import { headers } from "next/headers"

const Page = async () => {
  const timezone = (await headers()).get("x-vercel-ip-timezone")
  const commands = await main({ timezone: timezone || undefined })

  return <AnimatedBitmap commands={commands} />
}

export default Page
