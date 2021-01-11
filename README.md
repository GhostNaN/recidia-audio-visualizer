# ReCidia Audio Visualizer
### A highly customizable real time audio visualizer on Linux excluding Python
##### Based on ReVidia: https://github.com/GhostNaN/ReVidia-Audio-Visualizer


## Diffrences from ReVidia(Python)
The Good:
- No Python! It uses C/C++ now.
- Much faster, at least 3x times
- Way less RAM usage
- Lower latency
- Better terminal version 
- Proper config file "settings.cfg"

The Bad:
- Missing cool features
- No Windows support

## Dependencies
- gsl
  - Linear algebra
- glm
  - Vulkan linear algebra
- fftw 
  - Fast Fourier Transform
- ncurses
  - Teminal display
- libconfig
  - Config file manager
- shaderc
  - Runtime shader compilation
  
#### Must have at least one:
- portaudio(optional)
  - Audio data collection (Input Only)
- pulseaudio/pipewire-pulse(optional)
  - Audio data collection

## Installers
### Arch:
AUR package - https://aur.archlinux.org/packages/recidia-audio-visualizer/
 ## Building
 #### Requirements:
- meson
- ninja
- vulkan-headers

#### To build:
```
git clone --single-branch https://github.com/GhostNaN/recidia-audio-visualizer
cd recidia-audio-visualizer
meson build --prefix=/usr
ninja -C build
```
And if you wish to install:
```
ninja -C build install
mkdir ~/.config/recidia/
cp settings.cfg ~/.config/recidia/
cp -r shaders ~/.config/recidia/
```

## Usage 
### Running
Terminal version:
```
recidia
```
GUI version:
```
recidia literally any arg
```

### Customizing
Use the [settings.cfg](/settings.cfg) file to set: 
- default behavior 
- boundaries min/max
- keybindings/controls for runtime changes


 ## License
This project is licensed under the GPLv3 License - see the [LICENSE](/LICENSE) file for details
