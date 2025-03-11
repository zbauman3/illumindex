export const getToken = () => {
  const url = new URL(window.location.href);
  return url.searchParams.get('token') || '';
}

export const authFetch = (ep: string, config?: RequestInit) => {
  const url = new URL(ep, window.location.origin);
  const token = getToken();
  url.searchParams.set('token', token);
  return fetch(url, config);
}