# Qwixx Analyzer

## About

This project was developed by Andrew Landsbergen for his senior thesis as a partial fulfillment of requirements for the Bachelor of Science in Computer Science from Yale University. The project was completed on August 15th, 2025.

As this codebase was developed for a personal research project, it was not built with redistribution in mind. However, all source code is freely usable by any interested parties. There is no installer, and there is no guarantee that the project will build as-is on all platforms with all compilers. To have the highest chance of success at building the project on your own system, I would recommend having at least version 3.28 of CMake installed and at least version 13.3.0 of gcc installed. Then, after cloning the repository and navigating to the project root directory, run

```bash
mkdir build
cd build
cmake ..
cmake --build .
./QwixxAnalyzer
```

I am not a CMake expert, so if the ```cmake``` commands fail, you may need to troubleshoot independently. You can start by ensuring that

```bash
cmake --version
```

displays a version number of at least 3.28.

If you have Doxygen installed, an HTML file consisting of the project documentation can be generated with

```bash
doxgen Doxyfile
```

from the project root directory. You can then open ```html/index.html``` with a web browser to view the documentation.

## Contacting the Developer

If you would like access to the report that this codebase was used for, have questions about the codebase, or would like help with issues building on your system, you can try to contact me at the address alandsbergen2000@gmail.com. This email address is still in active use as of August 2025. Putting "[Qwixx Analyzer]" at the start of the subject line will help me find it faster.