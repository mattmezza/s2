# s2 website

Static HTML, small vanilla JavaScript enhancements, and compiled Tailwind CSS. Geist is loaded through Bunny Fonts. The screenshots are copied directly from `docs/screenshots`; no generated artwork or API key is needed.

```sh
npm ci --prefix site
npm run build --prefix site
npm run check --prefix site
npm run preview --prefix site
```

Preview at http://localhost:4173. From the repository root, `make site` installs the locked dependencies, builds, and checks the site. Build output is `site/dist/` and is intentionally ignored by Git.

`src/index.html` contains the marketing page; `build.mjs` produces the documentation directly from the repository README and controls guide, rewrites repository links, and emits metadata, a sitemap, robots.txt, llms.txt, and the Markdown documentation. `src/style.css` contains Tailwind's import and the app-inspired theme; `src/app.js` contains progressive enhancements. The site remains readable without JavaScript; animations honor reduced-motion settings.

The `pages.yml` workflow builds and deploys to GitHub Pages on pushes to `main`. In repository Settings → Pages, select **GitHub Actions** as the source. The canonical URL is https://matteo.merola.co/s2/ because this GitHub account's Pages domain is matteo.merola.co. The workflow uses relative asset paths so project hosting works correctly. No CNAME is needed in this project repository for the account domain.

A project-level `robots.txt` cannot override the origin's `/robots.txt`; the generated file documents the intended allow policy, while the account-level domain controls crawler access. All important content is server-delivered HTML.

Checks validate local links and fragments, duplicate IDs, structured data JSON, font configuration, required assets, and reduced-motion styles. Browser QA additionally covers desktop/mobile layouts, annotation previews, all theme controls, FAQ expansion, shortcut filtering, reduced motion, and JavaScript errors.
