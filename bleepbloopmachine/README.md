# bleepbloopmachine

## building

```sh
mkdir build
cd build
cmake -D CMAKE_TOOLCHAIN_FILE=/home/chee/projects/Arduino-CMake-Toolchain/Arduino-toolchain.cmake
-D CMAKE_EXPORT_COMPILE_COMMANDS=1  ..
ln build/compile_commands.json ..
```

then uncomment the pygamer lines in the generated `BoardOptions.cmake`, then run
the cmake command again and run `make` to build it.

## watching

if you keep this running, emacs will know about the new includes when you do an lsp-workspace-restart

```sh
watchexec -e cc,h -w .. 'cmake -D CMAKE_TOOLCHAIN_FILE=/home/chee/projects/Arduino-CMake-Toolchain/Arduino-toolchain.cmake -D CMAKE_EXPORT_COMPILE_COMMANDS=1 ..'
```
