import z from "zod";

export type TrashEvent = {
  trash?: {
    date: Date
    dayShort: string
    dayFull: string
  },
  recycling?: {
    date: Date
    dayShort: string
    dayFull: string
  }
}

export type TrashResponse = z.infer<typeof responseSchema>
const responseSchema = z.object({
  events: z.array(z.object({
    flags: z.array(
      z.object({
        subject: z.string()
      })
    ),
    day: z.string()
  }))
})

const dayShortStringMap = [
  'Sun',
  'Mon',
  'Tue',
  'Wed',
  'Thu',
  'Fri',
  'Sat',
]

const dayFullStringMap = [
  'Sunday',
  'Monday',
  'Tuesday',
  'Wednesday',
  'Thursday',
  'Friday',
  'Saturday',
]

export const trashDayCollector = async (): Promise<TrashEvent> => {
  // fake timezone of -5
  const after = new Date(Date.now() - (5 * 60 * 60 * 1000));
  const before = new Date(after.getTime() + (14 * 24 * 60 * 60 * 1000));


  const url = new URL('https://api.recollect.net/api/places/85317D08-B630-11E5-824D-AFD540878761/services/237/events');
  url.searchParams.set('nomerge', '1')
  url.searchParams.set('hide', 'reminder_only')
  url.searchParams.set('after', `${after.getFullYear()}-${after.getMonth() + 1}-${after.getDate()}`)
  url.searchParams.set('before', `${before.getFullYear()}-${before.getMonth() + 1}-${before.getDate()}`)
  url.searchParams.set('locale', 'en-US')
  url.searchParams.set('include_message', 'email')
  url.searchParams.set('_', `${Math.round(Date.now() / 1000)}`)

  const response = await fetch(url)

  if (!response.ok) {
    throw new Error(`Invalid response from ${url.host} - ${response.status}`)
  }

  const json = await response.json();
  const parsedResponse = responseSchema.parse(json);

  const events = parsedResponse.events.reduce<TrashEvent>((acc, event) => {
    const type = event.flags[0].subject
    let parsedType: keyof TrashEvent
    if (/trash|garbage/i.test(type)) {
      if (acc.trash) {
        return acc;
      }

      parsedType = 'trash';
    } else if (/recycling|recycle/i.test(type)) {
      if (acc.recycling) {
        return acc;
      }

      parsedType = 'recycling';
    } else {
      return acc;
    }

    const [year, monthIndex, date] = event.day.split('-');
    const parsedDate = new Date(parseInt(year), parseInt(monthIndex) - 1, parseInt(date), 12, 0, 0)
    const eventData: TrashEvent[keyof TrashEvent] = {
      date: parsedDate,
      dayFull: dayFullStringMap[parsedDate.getDay()],
      dayShort: dayShortStringMap[parsedDate.getDay()],
    }

    return {
      ...acc,
      [parsedType]: eventData
    }

  }, {} as TrashEvent)

  return events;
}