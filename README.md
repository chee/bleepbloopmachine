# bleepbloopmachine

## building

```sh
mkdir build
cd build
cmake -D CMAKE_TOOLCHAIN_FILE=/home/chee/projects/Arduino-CMake-Toolchain/Arduino-toolchain.cmake -D CMAKE_EXPORT_COMPILE_COMMANDS=1  ..
ln -s build/compile_commands.json ..
```

then uncomment the pygamer lines in the generated `BoardOptions.cmake`, then run
the cmake command again and run `make` to build it.

## watching

if you keep this running, emacs will know about the new includes when you do an lsp-workspace-restart

```sh
watchexec -e cpp,h -w .. 'cmake -D CMAKE_TOOLCHAIN_FILE=/home/chee/projects/Arduino-CMake-Toolchain/Arduino-toolchain.cmake -D CMAKE_EXPORT_COMPILE_COMMANDS=1 ..'
```

## uh ignore all that
i'm using arduino-builder now

## disregard that previous message
you can fix the problems with not being able to find libraries by symlinking the folder in ~/Arduino/libraries to whatever the header file is that can't be found

por exomp:

```sh
ln -s Adafruit_BusIO Adafruit_I2CDevice
```
