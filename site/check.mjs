import {readFile, access} from 'node:fs/promises';
import path from 'node:path';
let assertions=0;
function check(value,message) { if(!value) throw new Error(message); assertions++; }
for(const page of ['index.html','docs.html','404.html']) {
 const html=await readFile('dist/'+page,'utf8');
 check(!html.includes('{{'),'Unresolved template in '+page);
 const ids=[...html.matchAll(/\bid="([^"]+)"/g)].map(x=>x[1]);
 check(ids.length===new Set(ids).size,'Duplicate IDs in '+page);
 for(const match of html.matchAll(/(?:href|src)="([^"]+)"/g)) {
  const ref=match[1];
  if(/^(https?:|mailto:|data:)/.test(ref)) continue;
  const [file,fragment]=ref.split('#');
  const target=file==='./'?'index.html':file||page;
  await access(path.join('dist',target)); assertions++;
  if(fragment) { const linked=target===page?html:await readFile('dist/'+target,'utf8'); check(linked.includes(`id="${fragment}"`),`Missing anchor ${ref} in ${page}`); }
 }
 check(html.includes('fonts.bunny.net/css?family=geist'), 'Missing Geist font');
}
const home=await readFile('dist/index.html','utf8');
check(home.includes('https://matteo.merola.co/s2/'),'Canonical domain missing');
const structured=[...home.matchAll(/<script type="application\/ld\+json">(.*?)<\/script>/gs)];
for(const [,json] of structured) { JSON.parse(json); assertions++; }
for(const asset of ['assets/style.css','assets/app.js','sitemap.xml','llms.txt','docs.md','robots.txt','.nojekyll']) { await access('dist/'+asset); assertions++; }
check((await readFile('dist/assets/style.css','utf8')).includes('prefers-reduced-motion'), 'Reduced motion styles missing');
console.log(`Passed ${assertions} checks: local links, anchors, IDs, metadata, structured data, assets, and reduced-motion styles.`);
