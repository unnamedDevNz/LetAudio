# LetAudio
C++ library for playing audio. Part of LetEngine.

## Requirements
- C++23, CMake 3.28+
- Windows/Linux

## System Dependencies
- [OpenAL Soft](https://github.com/kcat/openal-soft): LGPL 
- [libsndfile](https://github.com/libsndfile/libsndfile): LGPL

## Possibilities
Now: only loading 2D audio entirely in memory and playing it.  
Planned: all commented from [LetAudio.hpp](./include/LetAudio.hpp)

## Documentation
Documentation builds via run `doxygen` from project root. (used theme - [doxygen-awesome-css](https://github.com/jothepro/doxygen-awesome-css): MIT)  
After build, you can open `build/html/index.html`

## License
[MIT](./LICENSE)