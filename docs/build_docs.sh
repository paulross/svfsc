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
# Doxygen PDF
pushd Sphinx/source/ref/doxygen/latex/
make pdf
# PDF is Sphinx/source/ref/doxygen/latex/refman.pdf so rename it.
mv refman.pdf svfs_doxygen.pdf
popd

# Sphinx
cd Sphinx
rm -rf build/
make html latexpdf
cd ..

echo ""
echo "Finished!"
echo "Sphinx HTML is in Sphinx/build/html/index.html"
echo "Sphinx PDF is in Sphinx/build/latex/svfsc.pdf"
echo "Doxygen HTML is copied into Sphinx/build/html/_static/doxygen_html/index.html"
echo "Doxygen PDF is in Sphinx/source/ref/doxygen/latex/svfs_doxygen.pdf"
