'use client';

import { useEffect, useState } from "react";

const getToken = () => {
  const url = new URL(window.location.href);
  return url.searchParams.get('token');
}


const sendCommands = async (commands: string) => {

  return fetch(
    `/api/command?token=${encodeURIComponent(getToken() || '')}`,
    { body: JSON.stringify({ commands: commands }), method: 'POST' }
  )
}

export default function Page() {
  const [value, setValue] = useState('');

  useEffect(() => {
    fetch(
      `/api/command?token=${encodeURIComponent(getToken() || '')}`,
      { method: 'GET' }
    ).then(async (response) => {
      setValue(JSON.stringify(await response.json(), null, 2))
    })
  }, [])

  return (
    <>
      <textarea style={{ width: '100%', }} value={value} onChange={(e) => { setValue(e.target.value); }}></textarea>
      <button onClick={() => { sendCommands(value); }}>Save</button>
    </>
  )
} 