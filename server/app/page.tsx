import { AnimatedBitmap } from "../components/AnimatedBitmap";
import { main } from "../main";

const Page = async () => {
  const commands = await main();

  return <AnimatedBitmap commands={commands} />
}

export default Page