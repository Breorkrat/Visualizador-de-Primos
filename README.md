## Prime Visualizer
A high-performance mathematical tool built in C and Raylib to visualize prime numbers (and multiples) in a polar coordinate system (r = θ = n). 

This project demonstrates the hidden patterns within the distribution of primes, revealing the "arms" and "spirals" that emerge from simple mathematical rules.

![Spiral Preview](screenshots/preview.gif)

## Technical Highlights
- **High Performance:** Uses a Segmented Sieve of Eratosthenes to calculate millions of primes on the fly with minimal memory footprint.
- **Advanced Culling:** Implements Binary Search to find visible primes and viewport culling to maintain high FPS while moving even with millions of points.
- **Precision:** Utilizes 64-bit integers and double-precision floating point coordinates to prevent "jitter" or "drifting" at astronomical distances.

## Controls

### Navigation
- **[WASD]**: Pan the camera
- **[Mouse Wheel]**: Zoom in/out
- **[R]**: Auto-zoom: Fill screen mode
- **[F]**: Auto-zoom: Full spiral view

### The Spiral
- **[Numbers 0-9]**: Type a number and press **[Enter]** to jump to a specific multiple.
- **[Left/Right Arrows]**: Increment/Decrement the current multiple (0 = Primes).
- **[Up/Down Arrows]**: Adjust generation speed (PPS).
- **[Enter]**: Pause / Resume generation.

### Visuals
- **[C]**: Cycle through Color Modes (Static, Breathing, etc.).
- **[TAB]**: Toggle Color Picker (Change custom colors for gradients).
- **[P]**: Save a high-resolution screenshot.
- **[F1/F2/F3]**: Toggle UI overlays (Controls, FPS, Stats).

### Gallery
| Prime Spiral (Green-Blue) | Multiples of 111 (Red-Black) |
| :-----------------------: | :--------------------------: |
| ![Primes](screenshots/preview.gif) | ![111s](screenshots/multiples_111.gif) |

## Installation & Build

### Linux
```bash
# Clone the repository
git clone https://github.com/Breorkrat/Prime-Visualizer.git
# Enter the directory
cd Prime-Visualizer
# Run the premake5 to download all needed dependencies
`./build/premake5 gmake`
# Then compile the program
`make`
# The program will be output in the bin folder, to run:
`./bin/Debug/Prime-Visualizer`
```

### VSCode (all platforms)
*Note* You must have a compiler toolchain installed in addition to vscode.
1. Download the project
2. Open the folder in VSCode
3. Run the build task (CTRL+SHIFT+B or F5)
5. You are good to go

### Windows
Make sure you have a modern version of MinGW-W64 (not mingw). The best place to get it is from the W64devkit from https://github.com/skeeto/w64devkit/releases or the version installed with the raylib installer
#### If you have installed raylib from the installer
Make sure you have added the path `C:\raylib\w64devkit\bin` to your path environment variable so that the compiler can be found

1. Download the project or use `git clone https://github.com/Breorkrat/Prime-Visualizer.git`
2. Double click the `build-MinGW-W64.bat` file
  * If you are using the W64devkit and have not added it to your system path environment variable, you must use the W64devkit.exe terminal, not CMD.exe
  * If you want to use CMD.exe or any other terminal, make sure gcc/mingw-W64 is in your path environment variable.
3. run `make`
4. The program will be compiled to .\bin\Debug\Prime-Visualizer.exe
