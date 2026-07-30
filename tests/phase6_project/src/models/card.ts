export interface Card {
  title: string;
}

export function makeCard(title: string): Card {
  return { title };
}
