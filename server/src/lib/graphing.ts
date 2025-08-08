/**
 * Generates graph values from a set of data, scaling them to fit within a specified range.
 * It calculates the maximum and minimum values from the data, and then scales the values
 * to fit within the range of `height`. It also groups the data into averages of `groupsOf`.
 */
export const generateGraphValues = ({
  data,
  defaultMax,
  defaultMin,
  height,
}: {
  /** The data to generate graph values for */
  data: number[]
  /**
   * The default "max" of the graph values. If a value in the set is greater than this, then it
   * will be used as the max instead.
   */
  defaultMax: number
  /**
   * The default "min" of the graph values. If a value in the set is less than this, then it
   * will be used as the min instead.
   */
  defaultMin: number
  /**
   * The scale to which the values should be mapped. For example, if `height` is 10, then the
   * values will be scaled to fit within the range of 0 to 9.
   */
  height: number
}): {
  data: number[]
  scaleMax: number
  scaleMin: number
  max: number
  min: number
} => {
  const scaleMax = Math.max(
    data.reduce((acc, temp) => (temp > acc ? temp : acc), 0),
    defaultMax
  )
  const scaleMin = Math.min(
    data.reduce((acc, temp) => (temp < acc ? temp : acc), 1000),
    defaultMin
  )
  const tempRange = scaleMax - scaleMin
  const scaled = data.map((temp) => {
    const valP = (temp - scaleMin) / tempRange
    return Math.round(valP * height)
  })

  return {
    scaleMax,
    scaleMin,
    max: data.reduce((acc, v) => Math.max(acc, v), scaleMin),
    min: data.reduce((acc, v) => Math.min(acc, v), scaleMax),
    data: scaled,
  }
}

/**
 * Scales the numbers to fit within the new width, ensuring that the values
 * are evenly distributed across the width.
 */
export const scaleUpGraphValues = ({
  data,
  width,
}: {
  /** The data to scale up */
  data: number[]
  /** The new width to scale the data to */
  width: number
}): number[] => {
  if (width < data.length) {
    throw new Error(
      "Width must be greater than or equal to the length of the data array."
    )
  }

  const scaleFactor = data.length / width
  const scaledData: number[] = []

  for (let i = 0; i < width; i++) {
    const index = Math.floor(i * scaleFactor)
    scaledData.push(data[index] || 0)
  }

  return scaledData
}

/**
 * It returns a new array of numbers that is the smoothed out version of the data.
 * It does this by averaging the values around each value in the data.
 */
export const smoothGraphValues = (data: number[]): number[] => {
  if (data.length < 3) {
    return data // Not enough data to smooth
  }

  const smoothedData: number[] = []
  for (let i = 0; i < data.length; i++) {
    const prev = data[i - 1] || data[i]
    const current = data[i]
    const next = data[i + 1] || data[i]

    const average = (prev + current + next) / 3
    smoothedData.push(Math.round(average))
  }

  return smoothedData
}
