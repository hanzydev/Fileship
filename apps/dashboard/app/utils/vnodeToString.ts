import { render, type VNode } from 'vue';

export const vnodeToString = (vnode: VNode) => {
    const { vueApp } = useNuxtApp();
    const container = document.createElement('div');

    vnode.appContext = vueApp._context;

    render(vnode, container);

    const html = container.innerHTML;

    render(null, container);

    return html;
};
