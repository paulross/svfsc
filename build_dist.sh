#!/bin/bash
#
# Builds svfs for distribution
# Ref: https://packaging.python.org/tutorials/packaging-projects/
#
# Other references:
# https://kvz.io/bash-best-practices.html
# https://bertvv.github.io/cheat-sheets/Bash.html
#
# This is a cut down version of build_all.sh that just builds and tests platform specific distributions.
# This merely omits the C++ build and C++ tests.

set -o errexit  # abort on nonzero exitstatus
set -o nounset  # abort on unbound variable
set -o pipefail # don't hide errors within pipes

# For current versions see https://devguide.python.org/versions/
PYTHON_VERSIONS=('3.8' '3.9' '3.10' '3.11' '3.12' '3.13')
# Used for venvs
PYTHON_VENV_ROOT="${HOME}/pyenvs"
PROJECT_NAME="svfsc"
CPP_EXECUTABLE="cppSVF"

#printf "%-8s %8s %10s %10s %12s\n" "Ext" "Files" "Lines" "Words" "Bytes"

deactivate_virtual_environment() {
  # https://stackoverflow.com/questions/42997258/virtualenv-activate-script-wont-run-in-bash-script-with-set-euo
  set +u
  if command -v deactivate &>/dev/null; then
    deactivate
  fi
  set -u
}

create_virtual_environments() {
  deactivate_virtual_environment
  for version in ${PYTHON_VERSIONS[*]}; do
    echo "---> Create virtual environment for Python version ${version}"
    venv_path="${PYTHON_VENV_ROOT}/${PROJECT_NAME}_${version}"
    if [ ! -d "${venv_path}" ]; then
      # Control will enter here if directory not exists.
      echo "---> Creating virtual environment at: ${venv_path}"
      "python${version}" -m venv "${venv_path}"
    fi
  done
}

remove_virtual_environments() {
  deactivate_virtual_environment
  for version in ${PYTHON_VERSIONS[*]}; do
    echo "---> For Python version ${version}"
    venv_path="${PYTHON_VENV_ROOT}/${PROJECT_NAME}_${version}"
    if [ -d "${venv_path}" ]; then
      # Control will enter here if directory exists.
      echo "---> Removing virtual environment at: ${venv_path}"
      #rm --recursive --force -- "${venv_path}"
      rm -rf -- "${venv_path}"
    fi
  done
}

create_and_test_bdist_wheel() {
  echo "---> Creating bdist_wheel for all versions..."
  for version in ${PYTHON_VERSIONS[*]}; do
    echo "---> For Python version ${version}"
    deactivate_virtual_environment
    venv_path="${PYTHON_VENV_ROOT}/${PROJECT_NAME}_${version}"
    if [ ! -d "${venv_path}" ]; then
      # Control will enter here if directory doesn't exist.
      echo "---> Creating virtual environment at: ${venv_path}"
      "python${version}" -m venv "${venv_path}"
    fi
    # https://stackoverflow.com/questions/42997258/virtualenv-activate-script-wont-run-in-bash-script-with-set-euo
    set +u
    source "${venv_path}/bin/activate"
    set -u
    echo "---> Python version:"
    python -VV
    echo "---> Installing everything via pip:"
    pip install -U pip setuptools wheel
    pip install -r requirements.txt
    # Needed for uploading to pypi
    pip install twine
    echo "---> Result of pip install:"
    pip list
    echo "---> Running python setup.py develop:"
#    MACOSX_DEPLOYMENT_TARGET=10.9 CC=clang CXX=clang++ python setup.py develop
    python setup.py develop
    echo "---> Running tests:"
    # Fail fast with -x
    pytest tests -x
    # Run all tests (slow).
    pytest tests --runslow --benchmark-sort=name
    echo "---> Running setup for bdist_wheel:"
    python setup.py bdist_wheel
  done
}

create_sdist() {
  echo "---> Running setup for sdist:"
  python setup.py sdist
}

create_documentation() {
  echo "---> Python version:"
  which python
  python -VV
  echo "---> pip list:"
  pip list
  echo "---> Building documentation:"
  cd docs
  ./build_docs.sh
  cd ..
  echo "---> Generating stub file:"
  python stubgen_simple.py
}

report_all_versions_and_setups() {
  echo "---> Reporting all versions..."
  for version in ${PYTHON_VERSIONS[*]}; do
    echo "---> For Python version ${version}"
    deactivate_virtual_environment
    venv_path="${PYTHON_VENV_ROOT}/${PROJECT_NAME}_${version}"
    if [ ! -d "${venv_path}" ]; then
      # Control will enter here if directory doesn't exist.
      echo "---> Creating virtual environment at: ${venv_path}"
      "python${version}" -m venv "${venv_path}"
    fi
      echo "---> Virtual environment at: ${venv_path}"
    # https://stackoverflow.com/questions/42997258/virtualenv-activate-script-wont-run-in-bash-script-with-set-euo
    set +u
    source "${venv_path}/bin/activate"
    set -u
    echo "---> Python version:"
    python -VV
    echo "---> pip list:"
    pip list
  done
# Don't do this as we want to use show_results_of_dist() and twine after this
#  deactivate_virtual_environment
}

show_results_of_dist() {
  echo "---> dist/:"
  ls -l "dist"
  echo "---> twine check dist/*:"
  twine check dist/*
  # Test from Test PyPi
  # pip install -i https://test.pypi.org/simple/orderedstructs
  echo "---> Ready for upload to test PyPi:"
  echo "---> pip install twine"
  echo "---> twine upload --repository testpypi dist/*"
  echo "---> Or PyPi:"
  echo "---> twine upload dist/*"
}

echo "===> Removing build/ and dist/"
#rm --recursive --force -- "build" "dist"
rm -rf -- "build" "dist"
echo "===> Removing virtual environments"
remove_virtual_environments
echo "===> Creating virtual environments"
create_virtual_environments
echo "===> Creating binary wheels"
create_and_test_bdist_wheel
echo "===> Creating source distribution"
create_sdist
echo "===> All versions and setups:"
report_all_versions_and_setups
echo "===> Building documentation:"
create_documentation
echo "===> dist/ result:"
show_results_of_dist
#deactivate_virtual_environment
echo "===> Date:"
date
echo "===> All done"
