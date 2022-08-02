# Yab2Cpp

A C++ export engine for the Yab interpreted language.

## Download Source Instructions

`git clone --recurse-submodules --depth 1 https://github.com/SamuraiCrow/Yab2Cpp.git`

If you forget to recurse-submodules or if the submodule code gets stale, type the following after it is done downloading:
`git submodule update --init --recursive`

## Compiling the Test Codes

Type the following in the Yab2Cpp directory and substitute the actual number of threads your CPU can run after the -j:
`make -j8 debug` or just `make -j8` for a release build.

Once it's done building, you can run the test code by typing:
`./tester` or `./tester -G` if you want it to generate all the build logs too.

To compile the executable, type `cd runtime` then `make -j8` and finally `./main` to try it out.

---

## Note:

The current version only supports test code for the code generator.  Progress is ongoing.
