# Native drawing performance

Run `python tests/motion.py` to exercise a 3840×2160 input under a private
1920×1080 Xvfb screen. It compares paced input with a burst of 100 pointer motions,
checks the PNG results are byte-identical, and prints frame timings using
`S2_PROFILE=1`. No user display is used. `--binary PATH` compares another build.

Representative results on this workspace (milliseconds per frame for the paced
input, including startup and the final committed frame):

| Tool | Previous build | Updated build |
| --- | ---: | ---: |
| Line | 12.6 | 3.4 |
| Pen | 25.7 | 2.9 |
| Marker | 28.4 | 4.0 |

These are CPU/X11 submission measurements on Xvfb, not compositor presentation
latency or a guarantee for every machine. Long strokes still retain every input
point. Coalescing only combines consecutive motion events, preserving releases,
keyboard events, and final committed geometry.

Use `python tests/smoke.py` for toolbar tools, undo/redo, selection/delete,
resizing/zoom, clipboard, and PNG export. `make test-blur` compares optimized blur
pixels against a straightforward neighborhood implementation over 400 randomized
regions, including clipped/reversed selections and varying radii.

The latest 4K paced-motion comparison on the same Xvfb host measured line preview average 3.93 → 1.22 ms, pen 3.70 → 1.59 ms, and marker 4.17 → 1.89 ms. Idle cursor movement rendered 12 frames instead of 105 during the same input sequence; native cursor movement is independent of these frames. Cached chrome, bounded motion batching, damage rectangles, and 10 Hz footer coordinates reduce redundant work. Initial layout/uploads and final commits are included in those averages and still have higher peaks. These numbers measure headless CPU/X11 submission, not desktop/compositor latency.
