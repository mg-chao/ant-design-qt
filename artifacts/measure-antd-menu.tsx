import fs from 'node:fs';
import path from 'node:path';
import React from 'react';
import ReactDOMServer from 'react-dom/server';
import { createCache, StyleProvider, extractStyle } from '@ant-design/cssinjs';
import Menu from './components/menu';
import { ConfigProvider } from './components';

const items = [
  { key: 'sub1', label: 'Navigation One', children: [ { key: '1', label: 'Option 1' }, { key: '2', label: 'Option 2' } ] },
  { key: 'sub2', label: 'Navigation Two', children: [ { key: '3', label: 'Option 3' }, { key: '4', label: 'Option 4' } ] },
];

const cache = createCache();
const app = (
  <StyleProvider cache={cache} hashPriority="high">
    <ConfigProvider>
      <Menu mode="inline" items={items} defaultOpenKeys={['sub1']} style={{ width: 256 }} />
    </ConfigProvider>
  </StyleProvider>
);

const html = ReactDOMServer.renderToString(app);
const styleText = extractStyle(cache);
const outPath = 'E:/ant-design-qt/artifacts/antd-menu-inline-measure.html';
fs.mkdirSync(path.dirname(outPath), { recursive: true });
fs.writeFileSync(outPath, `<!doctype html><html><head><meta charset="utf-8"><style>body{margin:0;padding:24px;background:#fff;}</style>${styleText}</head><body>${html}</body></html>`);
console.log(outPath);
