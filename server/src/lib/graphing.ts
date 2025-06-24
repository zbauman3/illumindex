const groupValues = (input: number[]): number[] => {
  const ret: number[] = []
  for (let group = 0; group < 12; group++) {
    let groupedValue = 0
    for (let value = 0; value < 2; value++) {
      groupedValue += input[group * 2 + value]!
    }
    ret.push(groupedValue / 2)
  }

  return ret
}

export const generateGraphValues = ({
  data,
  defaultMax,
  defaultMin,
  graphHeight,
}: {
  data: number[]
  defaultMax: number
  defaultMin: number
  graphHeight: number
}) => {
  const maxTemp = Math.max(
    data.reduce((acc, temp) => (temp > acc ? temp : acc), 0),
    defaultMax
  )
  const minTemp = Math.min(
    data.reduce((acc, temp) => (temp < acc ? temp : acc), 1000),
    defaultMin
  )
  const tempRange = maxTemp - minTemp
  const grouped = groupValues(data).map((temp) => {
    const valP = (temp - minTemp) / tempRange
    return Math.round(valP * (graphHeight - 1))
  })

  return grouped
}
