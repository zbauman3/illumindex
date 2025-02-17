export async function GET(request: Request) {
  const url = new URL(request.url)

  return Response.json({
    data: 'green',
    param: url.searchParams.get('param')
  })
}