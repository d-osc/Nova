import type { Card } from "@models/card";

export interface CardProps {
  card: Card;
}

export function CardView(props: CardProps) {
  return <article className="card">{props.card.title}</article>;
}

export const preview = <CardView card={{ title: "Nova" }} />;
