const reducedMotion = window.matchMedia('(prefers-reduced-motion: reduce)').matches;
if ('IntersectionObserver' in window && !reducedMotion) {
  const observer = new IntersectionObserver(entries => entries.forEach(entry => {
    if (entry.isIntersecting) { entry.target.classList.remove('waiting'); observer.unobserve(entry.target); }
  }), { threshold: 0.08 });
  document.querySelectorAll('.reveal').forEach(el => { el.classList.add('waiting'); observer.observe(el); });
}
document.querySelectorAll('[data-copy]').forEach(button => button.addEventListener('click', async () => {
  const original = button.textContent;
  try { await navigator.clipboard.writeText(document.getElementById(button.dataset.copy).textContent); button.textContent = 'Copied ✓'; }
  catch { button.textContent = 'Select the command to copy'; }
  setTimeout(() => { button.textContent = original; }, 2200);
}));
const tools = {
  arrow: ['Point to the exact detail.', 'Curved connectors, six end styles, and a head at either end.', '<path class="draw-line" d="M140 160 Q350 260 520 110" fill="none" stroke="#b3caff" stroke-width="5"/><path d="m497 110 25-4-8 25" fill="none" stroke="#b3caff" stroke-width="5"/>'],
  rectangle: ['Put a detail in the spotlight.', 'Independent outlines, fills, gradients, and custom colors.', '<rect x="135" y="98" width="390" height="110" rx="3" fill="#b3caff15" stroke="#b3caff" stroke-width="5"/>'],
  text: ['Say what the screenshot cannot.', 'Editable Geist text with weight, backgrounds, and typography controls.', '<rect x="168" y="105" width="325" height="72" rx="13" fill="#b3caff"/><text x="330" y="151" text-anchor="middle" fill="#10131b" font-size="29" font-family="Geist, sans-serif">This is the important bit.</text>'],
  number: ['Make the next steps obvious.', 'Numbered pins turn a screenshot into a walkthrough.', '<g fill="#b3caff"><circle cx="180" cy="115" r="26"/><circle cx="340" cy="175" r="26"/><circle cx="500" cy="110" r="26"/></g><g fill="#10131b" font-size="26" text-anchor="middle" font-family="Geist, sans-serif"><text x="180" y="124">1</text><text x="340" y="184">2</text><text x="500" y="119">3</text></g>'],
  highlight: ['Keep the context. Add emphasis.', 'Highlight regions or draw with a broad marker.', '<rect x="128" y="104" width="405" height="33" rx="2" fill="#facc1570"/><rect x="128" y="155" width="290" height="33" rx="2" fill="#facc1570"/>'],
  pixelate: ['Cover the details you do not need to share.', 'Blur and pixelate regions before copying or saving the rendered image.', '<g fill="#7c89a3"><path d="M145 104h40v40h-40zm80 0h40v40h-40zm80 0h40v40h-40zm80 0h40v40h-40zm80 0h40v40h-40z"/><path opacity=".4" d="M185 104h40v40h-40zm80 0h40v40h-40zm80 0h40v40h-40zm80 0h40v40h-40z"/><path opacity=".6" d="M145 144h40v40h-40zm80 0h40v40h-40zm80 0h40v40h-40zm80 0h40v40h-40zm80 0h40v40h-40z"/><path opacity=".25" d="M185 144h40v40h-40zm80 0h40v40h-40zm80 0h40v40h-40zm80 0h40v40h-40z"/></g>']
};
document.querySelectorAll('[data-tool]').forEach(button => button.addEventListener('click', () => {
  document.querySelectorAll('[data-tool]').forEach(el => el.setAttribute('aria-pressed', String(el === button)));
  const [title, description, drawing] = tools[button.dataset.tool];
  document.getElementById('demo-title').textContent = title;
  document.getElementById('demo-description').textContent = description;
  document.getElementById('demo-drawing').innerHTML = drawing;
}));
const themes = {
  Dark:['#0c0d0f','#181a20','#aab1c2','#b3caff','#3a3e4a'], Light:['#f4f5f7','#ffffff','#394150','#2459b8','#c4cbd7'], 'Tokyo Night':['#16161e','#1a1b26','#a9b1d6','#7aa2f7','#414868'], Gruvbox:['#1d2021','#282828','#ebdbb2','#fabd2f','#665c54'], Dracula:['#21222c','#282a36','#f8f8f2','#bd93f9','#6272a4'], Nord:['#242933','#2e3440','#d8dee9','#88c0d0','#4c566a'], Catppuccin:['#11111b','#1e1e2e','#cdd6f4','#cba6f7','#585b70'], Solarized:['#00212b','#002b36','#93a1a1','#2aa198','#586e75'], 'Rose Pine':['#191724','#1f1d2e','#e0def4','#ebbcba','#6e6a86']
};
document.querySelectorAll('[data-theme]').forEach(button => button.addEventListener('click', () => {
  document.querySelectorAll('[data-theme]').forEach(el => el.setAttribute('aria-pressed',String(el === button)));
  const preview = document.getElementById('theme-preview');
  ['workspace','panel','text','accent','border'].forEach((key,i) => preview.style.setProperty('--preview-'+key,themes[button.dataset.theme][i]));
  document.getElementById('theme-name').textContent = button.dataset.theme;
}));
const filter = document.getElementById('shortcut-search');
if (filter) filter.addEventListener('input', () => {
  let count = 0;
  document.querySelectorAll('#shortcuts tbody tr').forEach(row => { row.hidden = !row.textContent.toLowerCase().includes(filter.value.toLowerCase()); if (!row.hidden) count++; });
  document.getElementById('search-status').textContent = `${count} shortcuts shown`;
});
if ('IntersectionObserver' in window && document.querySelector('.docs-nav')) {
  const observer = new IntersectionObserver(entries => entries.forEach(entry => {
    if (entry.isIntersecting) { document.querySelectorAll('.docs-nav a').forEach(a => a.classList.toggle('active', a.hash === '#'+entry.target.id)); }
  }), {rootMargin:'-100px 0px -65% 0px'});
  document.querySelectorAll('.docs h2').forEach(el => observer.observe(el));
}
