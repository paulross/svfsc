#!/bin/bash
# Build all the documentation from scratch.
#
# Other references:
# https://kvz.io/bash-best-practices.html
# https://bertvv.github.io/cheat-sheets/Bash.html

set -o errexit  # abort on nonzero exitstatus
set -o nounset  # abort on unbound variable
set -o pipefail # don't hide errors within pipes

# Plots
cd plots/
gnuplot -p *.plt
cd ..

# Doxygen
doxygen SVFS.dox

# Sphinx
cd Sphinx
rm -rf build/
make html latexpdf
cd ..
