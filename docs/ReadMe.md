# Generating SVFS Documentation


The documentation is generated with Sphinx and Doxygen.
Sphinx is in ``requirements.txt``.

## Doxygen Documetation

From the project root directory:

```shell
cd docs
doxygen SVFS.dox
```

This will write the Doxygen HTML output to ``Sphinx/source/ref/doxygen/doxygen_html/``

## Gnuplot Plots

From the project root directory:

```shell
cd docs/plots
gnuplot -p *.plt
```

This will create all the plots to  ``docs/plots/`` which are referenced by the Sphinx documentation.

## Sphinx Documentation

From the project root directory:

```shell
cd docs/Sphinx
make html
open build/html/index.html
```

This will generate the documentation in ``build/html`` and copy the Doxygen documentation to ``build/html/_static/doxygen_html/``

To clean the build and create everything from scratch from the project root directory:

```shell
rm -rf docs/Sphinx/build/
cd docs
doxygen SVFS.dox
cd Sphinx
make html
open build/html/index.html
```
