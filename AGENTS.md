# AGENTS.md — Project agent rules

## Build directories are always untracked
- Build output directories (`build/`, `build_*/` such as `build_san`, `build_tr`,
  `build_tx`) and their artifacts MUST stay out of version control. They are
  excluded via `.gitignore`.
- Never stage, commit, or push anything under a build directory.
- If a new build directory is created, add it to `.gitignore` immediately. The
  `build_*/` glob already covers any `build_*` name, so prefer that pattern; add
  an explicit entry only if a non-`build_` name is introduced.
- `.kilo/` is also intentionally untracked.

## Commits
- Commit after every change that qualifies as one coherent thing. Multiple
  commits may be made per session; keep each incremental and focused.
- Never stage, commit, or push anything under a build directory.
