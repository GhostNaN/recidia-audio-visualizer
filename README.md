![Preview](https://github.com/GhostNaN/recidia-audio-visualizer/blob/assets/preview.apng)
# ReCidia Audio Visualizer
### A highly customizable real time audio visualizer on Linux
##### Based on ReVidia: https://github.com/GhostNaN/ReVidia-Audio-Visualizer

## Dependencies
- gsl
  - Linear algebra
- glm
  - Graphics linear algebra
- fftw 
  - Fast Fourier Transform
- ncurses
  - Teminal display
- libconfig
  - Config file manager
- shaderc
  - Runtime shader compilation
- qt5-base
  - GUI support
- vulkan-driver
  - Visualizer renderer
  
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
Use the [settings.cfg](/settings.cfg) file to: 
- set default behavior 
- adjust control boundaries
- set keybindings/controls for runtime changes
- read docs

 ## License
This project is licensed under the GPLv3 License - see the [LICENSE](/LICENSE) file for details
