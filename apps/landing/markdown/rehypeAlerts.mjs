const ALERT = /^!(NOTE|TIP|IMPORTANT|WARNING|CAUTION)$/;

const textOf = (node) =>
    node.type === 'text' ? node.value : (node.children ?? []).map(textOf).join('');

const toAlert = (node) => {
    const paragraph = node.children?.find((child) => child.type === 'element');
    if (paragraph?.tagName !== 'p') return;

    const marker = paragraph.children?.[0];
    if (marker?.type !== 'element' || marker.tagName !== 'span') return;

    const match = textOf(marker).trim().match(ALERT);
    if (!match) return;

    paragraph.children.shift();

    const next = paragraph.children[0];
    if (next?.type === 'text') next.value = next.value.replace(/^\s+/, '');

    node.tagName = 'docs-alert';
    node.properties = { type: match[1].toLowerCase() };
};

const visit = (node) => {
    if (node.type === 'element' && node.tagName === 'blockquote') toAlert(node);
    node.children?.forEach(visit);
};

export default () => (tree) => visit(tree);
