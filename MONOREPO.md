# ant-design-qt Monorepo

## Layout

```text
.
|- ant-design-qt.pro                     # workspace entry (SUBDIRS)
|- packages/
|  |- ant-design-qt/                     # existing component library
|  |  |- ant-design-qt.pro
|  |  `- src/
|  `- ant-design-icons-qt/               # new empty icon component library
|     |- ant-design-icons-qt.pro
|     |- resources/
|     `- src/
|- tools/
|  `- sync_ant_design_icons.py
`- examples/
   `- theme-demo/
      `- theme-demo.pro
```

## Build

- Build whole workspace from an out-of-source build directory:
  `qmake ../ant-design-qt.pro`
- Build only `ant-design-qt`:
  `qmake ../packages/ant-design-qt/ant-design-qt.pro`
- Build only `ant-design-icons-qt`:
  `qmake ../packages/ant-design-icons-qt/ant-design-icons-qt.pro`

## Icon Sync

- Sync assets and generated APIs from upstream:
  `python tools/sync_ant_design_icons.py --ref main`
- Dry run:
  `python tools/sync_ant_design_icons.py --ref main --dry-run`

## Notes

- `ant-design-icons-qt` now ships built-in SVG assets (`qrc`) and generated
  strong-typed icon APIs under `src/generated`.
- `examples/theme-demo` links against both `ant-design-qt` and
  `ant-design-icons-qt` outputs under package build directories.
