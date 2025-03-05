# nwge-bundle-tui

A `ncurses`-based TUI viewer for Nwge Bundle files.

Build with the `build.sh` script:

```sh
./build.sh
```

Make sure you have `ncurses`, `nwge_bndl` and `SDL2` available on your system.
You will get a `nwgebndl-tui` executable in the `target` directory. You can
simply copy this executable to, say, `~/.local/bin` and invoke it as
`nwgebndl-tui <bundle file name>`.

For example, running `nwgebndl-tui future.bndl` would yield:

![Screenshot of `nwgebndl-tui` showing the contents of
`future.bndl`](docs/screenshot.png)
