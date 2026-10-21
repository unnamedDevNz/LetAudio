# LetAudio
C++ library for playing audio. Part of LetEngine.

## Requirements
- C++23, CMake 3.28+
- Windows/Linux

## System Dependencies
- [OpenAL Soft](https://github.com/kcat/openal-soft): LGPL 
- [libsndfile](https://github.com/libsndfile/libsndfile): LGPL

## Possibilities
Now: decoding file entirely in memory, playing 2D audio.  
You can see what has already been done in [LetAudio.hpp](./include/LetAudio.hpp).  

Planned: Preload/Streaming, 2D/3D.   
You can see what is planned - everything that is commented in [LetAudio.hpp](./include/LetAudio.hpp).

## Documentation
Documentation builds via run `doxygen` from project root. (used theme - [doxygen-awesome-css](https://github.com/jothepro/doxygen-awesome-css): MIT)  
After build, you can open `build/html/index.html`

## License
[MIT](./LICENSE)