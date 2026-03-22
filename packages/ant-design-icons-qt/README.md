# ant-design-icons-qt

Qt6 icon library for Ant Design icons (`filled`, `outlined`, `twotone`).

## Features

- Full generated icon set from upstream `ant-design/ant-design-icons`
- Strongly-typed APIs only:
  - `adqt::icons::outlined::*`
  - `adqt::icons::filled::*`
  - `adqt::icons::twotone::*`
- Runtime SVG rendering with `QIconEngine`
- Theme-aware color resolver hooks
- Built-in `qrc` icon assets

## Sync from upstream

```bash
python tools/sync_ant_design_icons.py --ref main
```

Dry run:

```bash
python tools/sync_ant_design_icons.py --ref main --dry-run
```

After sync, generated files include:

- `resources/ant_design_icons.qrc`
- `resources/upstream.lock.json`
- `src/generated/icon_manifest.h/.cpp`
- `src/generated/icon_functions.h/.cpp`

## Basic usage

```cpp
#include "icons.h"

button->setIcon(adqt::icons::makeIcon(adqt::icons::outlined::Search()));
```

Custom two-tone colors:

```cpp
adqt::icons::IconStyle style;
style.primary = QColor("#1677FF");
style.secondary = QColor("#E6F4FF");
style.hasPrimary = true;
style.hasSecondary = true;

button->setIcon(adqt::icons::makeIcon(adqt::icons::twotone::Alert(style)));
```

## Theme integration

Install a resolver once:

```cpp
adqt::icons::setThemeResolver([]() {
  adqt::icons::IconThemeSnapshot s;
  s.text = QColor("#1F1F1F");
  s.textDisabled = QColor("#BFBFBF");
  s.primary = QColor("#1677FF");
  s.twoToneSecondary = QColor("#E6F4FF");
  s.revision = 1; // bump on theme change
  return s;
});
```

## Cache controls

```cpp
adqt::icons::setPixmapCacheLimitKB(32 * 1024);
adqt::icons::clearIconCache();
```
