# Lessons Learned

> Von Claude Code pflegbar. Kurze Einträge: Problem → Ursache → Lösung/Erkenntnis.
> Dient als Wissensspeicher für die Thesis (z. B. Kapitel „Herausforderungen").

### macOS: `liblzma`-Fehler beim Toolchain-Build (pioarduino)
**Problem:** PlatformIO bricht beim Laden/Entpacken der pioarduino-Toolchain mit
„Python's lzma module is unavailable/broken" ab.
**Ursache:** pioarduino-Toolchains sind `.tar.xz`-gepackt; PlatformIOs Python
braucht dafür `liblzma`, das auf macOS (insb. Apple Silicon) oft fehlt.
**Lösung:** Homebrew installieren, dann `brew install xz` (liefert
`/opt/homebrew/opt/xz/lib/liblzma.5.dylib`). Danach baut die Toolchain durch.
