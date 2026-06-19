---
name: display-preview
description: Render persistent preview images for embedded display environments before uploading firmware. Use when changing renderer code, bitmap masks, display geometry, orientation, gauges, or any src/<env> subproject where the user wants to inspect the visual result locally first.
---

# Display Preview

Use this skill before uploading visual display changes to hardware.

## Workflow

1. Identify the display environment, usually `src/<env>/`.
2. Prefer an existing preview script in `scripts/`. For standby compass, run:

```bash
python3 .agents/skills/display-preview/scripts/render_standby_compass_preview.py
```

3. Save previews under `previews/<env>/`. These files are ignored by git and should be treated as generated artifacts.
4. Keep previews in the renderer's raw display orientation by default. Only rotate when the user asks for a physical mounting orientation.
5. Show the persistent preview path to the user. Do not upload until the preview looks worth trying on hardware.
6. If no preview script exists for an env, create one by mirroring the renderer's geometry, source assets, colors, transforms, and flush orientation. Keep it deterministic and dependency-light.

## Rules

- Keep preview scripts close to firmware math. Duplicate constants intentionally when that makes the preview easy to audit.
- Use repo-local output paths such as `previews/standby_compass/current.png`.
- Do not commit generated previews.
- Make orientation explicit per project. Do not assume every display needs the same rotation.
- If the preview and hardware disagree, trust the preview only for renderer math; then inspect upload target, reset behavior, display rotation, and driver window coordinates separately.
