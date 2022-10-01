project('recidia', ['c', 'cpp'], default_options : ['cpp_std=c++17'])
cc = meson.get_compiler('cpp')

gsl = dependency('gsl')
fftw = dependency('fftw3')
threads = dependency('threads')
curses = dependency('ncursesw')
libconfig = dependency('libconfig++')
shaderc = dependency('shaderc')

audio_check = false

pipewire = dependency('libpipewire-0.3', required : false)
if pipewire.found()
    add_project_arguments('-D PIPEWIRE', language : 'c')
    audio_check = true
endif


pulse = dependency('libpulse', required : false)
pulse_simple = dependency('libpulse-simple', required : false)
if not pipewire.found()
    if pulse.found() and pulse_simple.found()
        add_project_arguments('-D PULSEAUDIO', language : 'c')
        audio_check = true
    endif
endif


portaudio = dependency('portaudio-2.0', required : false)
if portaudio.found()
    add_project_arguments('-D PORTAUDIO', language : 'c')
    audio_check = true
endif

assert(audio_check, 'You need PulseAudio and/or PortAudio')

qt6 = dependency('qt6', modules: ['Core', 'Gui', 'Widgets'])

# add_project_arguments('-march=native', '-mtune=generic', '-O1', '-pipe', '-fno-plt', '-fexceptions', '-Wp,-D_FORTIFY_SOURCE=2', '-Wformat', '-Werror=format-security', '-fstack-clash-protection', '-fcf-protection', language : 'cpp')

executable(meson.project_name(), ['src/main.cpp', 'src/audio.c', 'src/processing.cpp',
'src/curses.cpp', 'src/config.cpp', 'src/window.cpp', 'src/vulkan.cpp',
'src/widgets/devices.cpp', 'src/widgets/settings.cpp', 'src/widgets/stats.cpp'],
include_directories : ['inc'],
dependencies: [gsl, fftw, threads, curses, libconfig, pipewire, pulse_simple, portaudio, qt6, shaderc], install: true)
