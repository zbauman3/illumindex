// @ts-check

/**
 * This script updates the Vercel environment variable for the default state of Illumindex.
 * It reads the value from the .env.local file and sets it in Vercel for production.
 * Then it triggers a redeploy of the production environment.
 */

import { execSync } from "node:child_process"
import { type RemoteState } from "@/data/remoteState"

const varName = "ILLUMINDEX_DEFAULT_STATE"
const deployment = "illumindex.vercel.app"
const commandPath = "/api/command"

const main = async () => {
  const command = process.argv[2]
  if (command !== "dev-mode" && command !== "clear") {
    console.error('Invalid command. Use "dev-mode" or "clear".')
    return
  }

  if (command === "clear") {
    const clearCommand = `npx vercel env rm ${varName} production --yes`
    console.log(clearCommand)
    execSync(clearCommand, { stdio: "inherit" })
  } else {
    const localIp = execSync(
      `ipconfig getifaddr en0 || ipconfig getifaddr en1`,
      { stdio: "pipe" }
    )
      .toString()
      .trim()
    const commandEndpoint = `http://${localIp}:3000${commandPath}`

    const pushState: RemoteState = {
      commandEndpoint,
      fetchInterval: 10,
      isDevMode: true,
    }
    const envVarValue = JSON.stringify(pushState)

    const varCommand = `echo '${envVarValue}' | npx vercel env add --force ${varName} production`
    console.log(varCommand)
    execSync(varCommand, { stdio: "inherit" })
  }

  const redeployCommand = `npx vercel redeploy ${deployment}`
  console.log(redeployCommand)
  execSync(redeployCommand, { stdio: "inherit" })
}

if (/node$/.test(process.argv[0]) && /updateEnv\.ts$/.test(process.argv[1])) {
  main()
}
