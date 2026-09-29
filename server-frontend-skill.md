---
name: midnight-emerald-ui
description: Visual style guide for building frontends in a calm, dark "midnight + emerald" aesthetic — deep navy-black surfaces, hairline borders, restrained emerald accents, monospace numerals, and quiet tinted badges. Use whenever the user asks for a UI, dashboard, admin panel, settings page, landing page, or component "in my style", "in the dark emerald style", or references this skill by name.
Midnight Emerald UI
A quiet, flat, developer-tool aesthetic. Dark by default, almost no shadows, structure comes from hairline borders and subtle surface steps rather than contrast. Color is rare and meaningful: emerald means "active / good / live", everything else is grayscale.
The feeling to aim for: calm, precise, technical, expensive. Think Vercel, Linear, Supabase dashboards. If a screen feels loud, remove color before adding anything.
Core principles
Dark, layered, flat. Three surface levels max. No drop shadows, no glassmorphism, no glows.
Hairlines over boxes. Separate with 1px low-contrast borders and dividers, not heavy backgrounds.
One accent. Emerald is the only saturated color used for UI state. Other hues appear only as small data/category tints.
Small type, lots of air. Body text is 13–14px. Generous padding makes it feel premium instead of cramped.
Numbers are mono. Any metric, count, percentage, or code-like value uses a monospace font.
Tint, don't fill. Badges and icon tiles use the accent at ~12–18% opacity with full-strength text/icon on top.
Color tokens
```css
:root {
  /* Surfaces */
  --bg:            #070b14;  /* app background, near-black navy */
  --surface:       #0b111d;  /* cards, sidebar */
  --surface-2:     #111827;  /* rows, inputs, segmented control track, hover */
  --surface-3:     #1f2937;  /* selected segment, pressed */

  /* Lines */
  --border:        #1a2230;  /* card outlines */
  --divider:       #1e2633;  /* inner separators */

  /* Text */
  --text:          #f1f5f9;  /* headings, key numbers */
  --text-muted:    #94a3b8;  /* labels, secondary copy */
  --text-subtle:   #64748b;  /* axis labels, captions */
  --text-disabled: #3b4556;  /* unavailable nav items */

  /* Accent */
  --accent:        #10b981;  /* emerald */
  --accent-strong: #34d399;  /* accent text on dark tint */
  --accent-tint:   rgba(16, 185, 129, 0.14);

  /* Secondary data hues (use sparingly, for categories only) */
  --blue:          #3b82f6;
  --blue-tint:     rgba(59, 130, 246, 0.16);
  --violet:        #a855f7;
  --violet-tint:   rgba(168, 85, 247, 0.16);
  --neutral-tint:  rgba(148, 163, 184, 0.12);
}
```
Rules:
Never put pure white (#fff) or pure black (#000) on screen.
The accent never covers a large area. Largest emerald area allowed: a chart fill fading to transparent.
Red/amber exist only for real errors/warnings, same tint pattern.
Typography
Sans: `Inter`, fallback `ui-sans-serif, system-ui, -apple-system, sans-serif`.
Mono: `JetBrains Mono` or `Geist Mono`, fallback `ui-monospace, SFMono-Regular, Menlo, monospace`.
Role	Size	Weight	Color	Notes
Page/card title	18px	600	`--text`	
Section title	15px	500	`--text`	
Body / labels	13–14px	400–500	`--text-muted`	
Captions, axis	12px	400	`--text-subtle`	
Eyebrow label	12px	500	`--text-muted`	`text-transform: uppercase; letter-spacing: 0.06em`
Metric value	18–22px	600	`--text`	mono, `font-variant-numeric: tabular-nums`
Delta / percent	12px	500	`--accent-strong`	mono
Pair a big mono number with a tiny muted word beside it ("104 Current") rather than a label above it when space is tight.
Spacing, radius, borders
Spacing scale: 4, 8, 12, 16, 24, 32. Card padding is 24px. Gap between cards 16px.
Radius: cards `12px`, rows/inputs/buttons `8px`, icon tiles `6px`, badges `9999px`.
Every card: `background: var(--surface); border: 1px solid var(--border);`. No shadow.
Inner dividers: `border-top: 1px solid var(--divider)`, full width of card content.
Components
Sidebar nav
Items: 40px tall, 12px horizontal padding, 8px radius, icon 16px + label.
Active: `--surface-2` background, emerald text and icon, and a 3px emerald bar on the left edge (rounded).
Disabled: `--text-disabled` for icon and label, with a neutral "Coming soon" pill.
Utility controls (theme toggle etc.) pinned to the bottom as small 32px icon buttons; the selected one gets an accent tint.
Badges / pills
20–22px tall, 8px horizontal padding, 12px text, fully rounded.
Positive: `--accent-tint` bg, `--accent-strong` text. Neutral: `--neutral-tint` bg, `--text-muted` text.
Status variant: no background, a 6px colored dot followed by text.
Segmented control
Track: `--surface-2`, 8px radius, 4px inner padding.
Segments: 13px text in `--text-muted`; selected segment gets `--surface-3` bg and `--text`. No accent color here.
Icon tiles
28–32px square, 6px radius, category tint background, 16px icon in the full hue.
List rows (stat rows, table-like lists)
Left: icon tile + eyebrow label with mono value below. Right: muted descriptive text, right-aligned.
Rows separated by dividers, or placed on `--surface-2` with 8px radius and 8px gaps when they're interactive.
Right-aligned numbers are mono and `--text`.
Buttons
Primary: emerald bg, near-black text (`#04130d`), 8px radius, 36px tall. Use at most one per view.
Secondary: transparent, 1px `--border`, `--text` label, hover `--surface-2`.
Icon buttons: 32px, no border, `--text-muted`, hover `--surface-2`.
Charts
No gridlines, no chart borders, no legends inside the plot. Axis labels only, 12px `--text-subtle`.
Primary series: 2px emerald line, area fill as a vertical gradient from `rgba(16,185,129,0.35)` to transparent.
Comparison series: 1.5px slate line (`#64748b`), faint fill.
Legend lives above the chart as dot + label + mono value.
Donuts: thick ring (~20% of diameter), small gap between segments, no labels on the ring.
Interaction
Transitions: 150ms ease on background, color, border-color. Nothing bounces or scales.
Hover: step one surface level up. Focus: 2px ring in `rgba(16,185,129,0.5)` with 2px offset.
Loading: subtle `--surface-2` skeleton blocks, no spinners except in small icon buttons.
Do / Don't
Do: keep most of the screen grayscale, align everything to a clear grid, right-align numbers, use mono for all figures, let whitespace do the work.
Don't: gradients on backgrounds or buttons, drop shadows, glow effects, multiple accent colors for state, emoji in UI, bold everywhere, centered body text, borders thicker than 1px.
Light mode (if requested)
Keep the same structure and invert surfaces: `--bg #f8fafc`, `--surface #ffffff`, `--surface-2 #f1f5f9`, `--border #e2e8f0`, `--text #0f172a`, `--text-muted #475569`. Emerald shifts to `#059669` for contrast; tints stay at the same opacity.
