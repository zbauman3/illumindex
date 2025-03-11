'use client';

import { useEffect, useState } from "react";

import { commandsArray } from "../../lib/commands";
import { authFetch } from "../../lib/fetcher";

const sendCommands = (commands: string) => {
  const parsed = commandsArray.parse(JSON.parse(commands));
  return authFetch(`/api/command`, {
    body: JSON.stringify({ commands: JSON.stringify(parsed) }),
    method: 'POST'
  });
}

const getCommands = () =>
  authFetch(`/api/command`, { method: 'GET' })
    .then((res) => res.json())
    .then((json) => commandsArray.parse(json))

export default function Page() {
  const [value, setValue] = useState('');
  const [isLoading, setIsLoading] = useState(false);

  const updateCommands = async () => {
    try {
      setIsLoading(true);
      const commands = await getCommands();
      setValue(JSON.stringify(commands, null, 2));
    } catch (e) {
      console.log(e);
      alert(`Error getting commands.`);
    } finally {
      setIsLoading(false);
    }
  }

  const saveCommands = async () => {
    try {
      setIsLoading(true);
      await sendCommands(value);
      await updateCommands();
    } catch (e) {
      console.log(e);
      alert(`Error setting commands.`);
    } finally {
      setIsLoading(false);
    }
  }

  useEffect(() => {
    updateCommands();
  }, [])

  return (
    <>
      <textarea
        style={{ width: '100%', minHeight: 500 }}
        disabled={isLoading}
        value={value} onChange={(e) => { setValue(e.target.value); }}
      />
      <button onClick={saveCommands} disabled={isLoading}>Save</button>
    </>
  )
} 