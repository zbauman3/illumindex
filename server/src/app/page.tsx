import { AnimatedBitmap } from "@/components/AnimatedBitmap"
import { main } from "@/main"

const Page = async () => {
  const apiResp = await main()

  return <AnimatedBitmap {...apiResp} />
}

export default Page
