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
- External SVG icon registration for app-specific icons

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

## Custom icons

External projects can register SVG icons and receive the same `IconToken` type used by
the generated Ant Design icon APIs. Registered icons participate in the same rendering,
DPR, cache, disabled-state, and theme color pipeline.

For single-color custom icons, prefer `fill="currentColor"` so widgets can inherit
their content color. Custom icons also expose metadata through `iconMetadata`,
`isSingleTone`, and `isTwoTone`; widgets should use these APIs instead of reaching
into generated manifests.

```cpp
namespace snow::icons::outlined {

inline adqt::icons::IconToken Select() {
  static const adqt::icons::IconToken token = adqt::icons::registerSvgIcon(
      adqt::icons::IconTheme::Outlined,
      QStringLiteral("snow-select"),
      QByteArrayLiteral(
          R"(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 1024 1024"><path fill="currentColor" d="M128 128h768v768H128z"/></svg>)"));
  return token;
}

}  // namespace snow::icons::outlined

button->setIcon(adqt::icons::makeIcon(snow::icons::outlined::Select()));
```

SVGs can also be registered from disk or Qt resources:

```cpp
auto fileIcon = adqt::icons::registerSvgIconFile(
    adqt::icons::IconTheme::Outlined, "snow-file", "/path/to/icon.svg");
auto qrcIcon = adqt::icons::registerSvgIconResource(
    adqt::icons::IconTheme::Outlined, "snow-qrc", ":/snow/icons/select.svg");
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
