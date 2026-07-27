// NOVA_TEST_MODE: compile
// NOVA_EXPECT_EXIT: 0

declare namespace JSX {
    interface IntrinsicElements {
        div: { id?: string; children?: unknown };
        span: { className?: string; children?: unknown };
    }
}

interface CardProps {
    title: string;
}

function Card(props: CardProps) {
    return <div id="card"><span>{props.title}</span></div>;
}

const view = <Card title="Nova" />;
