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

## Sphinx Documentation

From the project root directory:

```shell
cd docs/Sphinx
make html
open build/html/index.html
```

This will generate the documentation in ``build/html`` and copy the Doxygen documentation to ``build/html/_static/doxygen_html/``
