# How to contribute

Thanks for wanting to help. The project is under way and every test, report or fix counts. The documentation is in English; issues and PRs are welcome in English or Spanish.

## The rule that is never broken

**Nothing from the game goes into the repository or the issues:** no ISO, no ELF, no IRX, no disc files, no C++ from `generated/`, no VU1 or IOP captures (`out/capturas/`), no save files. They are already in `.gitignore`; if `git status` shows you any of that, don't add it. Issues that ask for or link to copies of the game get closed.

To show a graphics glitch, describe the scene or link to a video of your own hosted outside the repo.

## Reporting a bug

Open an issue with:

1. **Your ELF's SHA1** (printed by `scripts/extract-iso.ps1`). If it doesn't match the one in [docs/NOTES.md](docs/NOTES.md), the addresses aren't valid for your copy: say so in the issue, but don't expect it to work.
2. **Commit** of the repo and of the submodule (`git rev-parse --short HEAD` and `git -C PS2Recomp rev-parse --short HEAD`).
3. Whether or not you have the **native VU1 and IOP** generated (SETUP.md, 7b).
4. What you did, what you expected and what happened. The **last lines of the log** in `out/logs/` (text only) help a lot.

Before that, check the blockers table and "Repository status" in [docs/NOTES.md](docs/NOTES.md): it may already be noted there.

## Sending a fix

1. Fork the repo and make one branch per change.
2. **First try an override** (`overrides/`). If the fix belongs in the runtime, it goes in the fork [DaniSBR1/PS2Recomp](https://github.com/DaniSBR1/PS2Recomp), branch `godofwar2`: open the PR there (code comments in English) and another one here with the new submodule pointer.
3. Addresses **are only valid for the reference ELF** (SHA1 in NOTES.md). GoW1's addresses are not valid here.
4. If you find something (a function, the cause of a blocker), **write it down in `docs/NOTES.md`** in the same PR. That is what lets others pick up where you left off.
5. Temporary hacks (`ret0`, skipping a function…) carry a `TEMPORAL` comment and a row in NOTES.md.
6. No diagnostic traces in the runtime: keep them in your local copy.

PRs from outside the team are reviewed by a team member before they are merged.

## License

By contributing you agree that your contribution is published under **GPLv3**, like the rest of the project. If you bring in code from another project, it must be GPLv3-compatible and credit its source in a comment.
