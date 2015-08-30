# lib-lifx

lib-lifx is a C++14 library for controlling LIFX devices. This library also includes a CLI tool for controlling LIFX lightbulbs, which can be used as an example for usage of the library.

### Requirements
- [Premake5](https://premake.github.io/download.html)
- C++14 compiler

### Compiling
To compile lib-lifx, init & update the git submodules, then premake with the user-preferred action in the root directory of the git checkout. Afterwards, you can compile in the build directory. When compiling, unit tests will run automatically to ensure the library is functioning correctly.

For example:
1. `git submodule init`
2. `git submodule update`
3. `premake5 gmake`
4. `cd build`
5. `make -j`

### License
[MIT](http://codemaster.mit-license.org)
