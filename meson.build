project('recidia', ['c', 'cpp'], default_options : ['cpp_std=c++11'])
cc = meson.get_compiler('cpp')

gsl = dependency('gsl')
fftw = dependency('fftw3')
threads = dependency('threads')
curses = dependency('ncursesw')
libconfig = dependency('libconfig++')
shaderc = dependency('shaderc')

audio_check = false

pulse = dependency('libpulse', required : false)
pulse_simple = dependency('libpulse-simple', required : false)
if pulse.found() and pulse_simple.found()
    add_project_arguments('-D PULSEAUDIO', language : 'c')
    audio_check = true
endif

portaudio = dependency('portaudio-2.0', required : false)
if portaudio.found()
    add_project_arguments('-D PORTAUDIO', language : 'c')
    audio_check = true
endif

assert(audio_check, 'You need PulseAudio and/or PortAudio')

qt5 = dependency('qt5', modules: ['Core', 'Gui', 'Widgets'])

#add_project_arguments('-D_FORTIFY_SOURCE=2', '-O2', language : 'cpp')

executable(meson.project_name(), ['src/main.cpp', 'src/audio.c', 'src/processing.cpp',
'src/curses.cpp', 'src/config.cpp', 'src/window.cpp', 'src/vulkan.cpp', 'src/settings_gui.cpp'],
include_directories : ['inc'],
dependencies: [gsl, fftw, threads, curses, libconfig, pulse_simple, portaudio, qt5, shaderc], install: true)
