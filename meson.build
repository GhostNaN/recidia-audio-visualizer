project('recidia', ['c', 'cpp'], default_options : ['cpp_std=c++11'])
cc = meson.get_compiler('cpp')

gsl = dependency('gsl')
fftw = dependency('fftw3')
threads = dependency('threads')
curses = dependency('ncursesw')
libconfig = dependency('libconfig++')
portaudio = dependency('portaudio-2.0')

pulse = dependency('libpulse', required : false)
if pulse.found()
    add_project_arguments('-D PULSE', language : 'c')
endif

executable(meson.project_name(), ['src/main.cpp', 'src/audio.c', 'src/processing.cpp', 'src/curses.cpp', 'src/config.cpp'],
include_directories : ['inc'],
dependencies: [gsl, fftw, threads, curses, libconfig, portaudio, pulse], install: true)
