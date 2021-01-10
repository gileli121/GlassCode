# GlassIDE
This plugin allows you to make your IDE to be fully transparent while keeping the code sharp
and bright.

## Introduction
Adjust the opacity level while keeping the text sharp and bright
![][file:glass-ide-change-opacity.gif]

Using GlassIDE plugin with combination of LiveEdit (WebStorm IDE)
![][file:live-edit-with-glass-ide.gif]

Using GlassIDE in Java (IntelliJ IDE)
![][file:glass-ide-java-preview-1.png]

Using GlassIDE with blur effect set to high
![][file:glass-ide-java-high-blur-preview.png]

## Getting started

1. Download GlassIDE plugin
2. Install it in your IDE
3. It is recommended to use the high contrast theme for the best result or at lest any dark theme.
My suggestion is to enable the high contrast theme

## How to use
Currently, the only way to use this plugin is to click on the GlassIDE tab here: 
![][file:glass-ide-menu.png]

After that, the following menu will open:
![][file:glass-ide-menu-panel.png]

Click on the `Enable` checkbox to enable the effect.

In this menu, the following options available:

* `Opacity` - This is the opacity of the background only. The image processing algorithm is smart to not apply the opacity to the texts or anything that is not background
* `Amount of brightness behind the window` - This will reduce or decrease the light that you can see behind the window. This effect give similar result like when the opacity is high value and the theme is dark. In general it is suggested to set this filter to 70% and the opacity to 30%-60% (it depends on what you like)
* `Type of blur behind the window` - This effect have 3 options: `None`, `Medium` and `High`.
<br>
When it set to `None`, there is no blur effect at all. For `Medium` and `High` it looks as following:
<br>

#### Medium blur:
![][file:glass-ide-blur-medium.png]
#### High blur:
![][file:glass-ide-blur-high.png]

## Supported IDEs
* IntelliJ
* WebStorm
* Phpstorm
* Other JetBrains IDEs (Not tested)

## Supported Operating Systems
* Windows 10 64bit from version 2004 (20H1) and above

## Notes
* Currently this plugin is supported only in Windows 10 v2004 and above
* It is highly recommended to use the "High contrast" theme when using the glass effect. The transparency algorithm works the best on high contrast theme such as the.
* The source code for `bin/Renderer.exe` is not available. My decision was not to expose the complex image processing algorithm that used to produce the effect.



[file:glass-ide-change-opacity.gif]: .github/readme/glass-ide-change-opacity.gif
[file:live-edit-with-glass-ide.gif]: .github/readme/live-edit-with-glass-ide.gif
[file:glass-ide-java-preview-1.png]: .github/readme/glass-ide-java-preview-1.png
[file:glass-ide-java-high-blur-preview.png]: .github/readme/glass-ide-java-high-blur-preview.png
[file:glass-ide-menu.png]: .github/readme/glass-ide-menu.png
[file:glass-ide-menu-panel.png]: .github/readme/glass-ide-menu-panel.png
[file:glass-ide-blur-medium.png]: .github/readme/glass-ide-blur-medium.png
[file:glass-ide-blur-high.png]: .github/readme/glass-ide-blur-high.png