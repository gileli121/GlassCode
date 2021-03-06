# GlassCode
This plugin allows you to make your IDE to be fully transparent while keeping the code sharp
and bright.

See for example how it can be used for editing websites:
https://www.youtube.com/watch?v=mbVLot2m6FU

## Introduction
Adjust the opacity level while keeping the text sharp and bright
![][file:glass-ide-change-opacity.gif]

Using GlassCode plugin with combination of LiveEdit (WebStorm IDE)
![][file:live-edit-with-glass-ide.gif]

Using GlassCode in Java (IntelliJ IDE)
![][file:glass-ide-java-preview-1.png]

Using GlassCode with blur effect set to high
![][file:glass-ide-java-high-blur-preview.png]

## Getting started

1. Download GlassCode plugin
2. Install it in your IDE
3. It is recommended to use the high contrast theme for the best result or at lest any dark theme.
My suggestion is to enable the high contrast theme



## Super important!
To get high performance, go to: File->Settings 
And make sure you enabled "Enable GPU Acceleration (For NVIDIA CUDA Enabled GPUs only)" 
![image](https://user-images.githubusercontent.com/17680514/116011605-a1e86300-a62e-11eb-9e01-fef23b162160.png)
This option will work only for NVIDIA GPUs 

Otherwise, you may have to reduce the screen resolution to maximum 1920x1080 because the effect may not work fast enough without GPU Acceleration.

If you think that you can optimize the performance for CPU (without GPU Acceleration), feel free to design a better algorithm here:
https://github.com/gileli121/GlassCode/blob/80150bd18be2118cfde4b3943950ba65d8746dc8/CPP/process_layer_cpu.cpp#L690
Replace this function with your faster algorithm and submit PR.
The function is responsible for detecting the texts(or any shapes...) so that when the opacity applied, the opacity will not include the texts.

The result will look like this
![image](https://user-images.githubusercontent.com/17680514/116011894-f9d39980-a62f-11eb-931d-489effaf5f4a.png)
That the texts/shapes have no opacity at all but everything around it have opacity. 

The magic was done here:

CPU algorithm: https://github.com/gileli121/GlassCode/blob/80150bd18be2118cfde4b3943950ba65d8746dc8/CPP/process_layer_cpu.cpp#L690

NVIDIA GPU algorithm (works much faster!): https://github.com/gileli121/GlassCode/blob/1c89a644bae2b2c51f274b3ad1aee07fb0d36ae8/CPP/process_layer_gpu.cu#L433



## How to use
Currently, the only way to use this plugin is to click on the GlassCode tab here:

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

* `Use "High Contrast" theme when enabled (Recommended)` - The transparency algorithm works best when the number of colors in the theme is minimum. The algorithm will detect much better texts, borders, and backgrounds in such cases. Here is an example that compares the difference between when it is enabled and when it is not:

### High Contrast mode enabled:
![][file:high-contrast-enabled.png]

### High Contrast mode disabled:
![][file:high-contrast-disabled.png]

* `Enable on startup` - If this option is enabled, the transparency effect will be enabled on startup when the IDE open the project


## Settings Page
The settings page is available under File -> Settings Appearance & Behavior
![][file:plugin-settings-page.png]

In the following settings page you have the following options:

* `Enable GPU Acceleration (For NVIDIA CUDA Enabled GPUs Only)` - If this option is enabled, the transparency algorithm will use the GPU processing power to render the transparency effect. This option is very likely to improve a lot the number of frames per second (FPS). 

    > NOTE: If in the computer there is no NVIDIA GPU with CUDA support, this option will be ignored


## Supported IDEs
* IntelliJ (for Java)
* WebStorm (for Web & HTML)
* Phpstorm (for PHP)
* Rider (for C#)
* PyCharm (for Python)
* Other JetBrains IDEs (Not tested)

## Supported Operating Systems
* Windows 10 64bit from version 2004 (20H1) and above

## Notes
* Currently this plugin is supported only in Windows 10 v2004 and above
* It is highly recommended to use the "High contrast" theme when using the glass effect. The transparency



[file:glass-ide-change-opacity.gif]: .github/readme/glass-ide-change-opacity.gif
[file:live-edit-with-glass-ide.gif]: .github/readme/live-edit-with-glass-ide.gif
[file:glass-ide-java-preview-1.png]: .github/readme/glass-ide-java-preview-1.png
[file:glass-ide-java-high-blur-preview.png]: .github/readme/glass-ide-java-high-blur-preview.png
[file:glass-ide-menu.png]: .github/readme/glass-ide-menu.png
[file:glass-ide-menu-panel.png]: .github/readme/glass-ide-menu-panel.png
[file:glass-ide-blur-medium.png]: .github/readme/glass-ide-blur-medium.png
[file:glass-ide-blur-high.png]: .github/readme/glass-ide-blur-high.png
[file:high-contrast-disabled.png]: .github/readme/high-contrast-disabled.png
[file:high-contrast-enabled.png]: .github/readme/high-contrast-enabled.png
[file:plugin-settings-page.png]: .github/readme/plugin-settings-page.png


