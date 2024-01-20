"""
A simple bit of code that inspects a Python C Extension and generates a stub file for typing.
This expects the docstrings of each object to be of a specific form where the first line is the signature.
"""
import datetime
import inspect
import os
import pprint
import typing

import svfsc


def get_first_signature_line_from_docs(docs: typing.List[str]) -> str:
    """Recover the first line from the doc string as a signature.
    Acceptable forms are::

        def foo(a: int = 0) -> int:
        foo(a: int = 0) -> int
    """
    sig_line = docs[0].strip()
    if sig_line.startswith('def '):
        sig_line = sig_line[4:]
    if sig_line.endswith(':'):
        sig_line = sig_line[:-1]
    return sig_line


def write_members_recursively(obj, prefix: str, inc_docs: bool, file: typing.TextIO):
    """Take and object and write the stub file signatures.
    This is recursive so that a module will write its classes, a class will write its methods and so on.
    """
    members = inspect.getmembers(obj)
    suffix = '' if inc_docs else ' ...'
    for name, member in members:
        if name.startswith('__'):
            continue
        if inspect.isclass(member):  # and not name.startswith('__'):
            file.write(f'\n{prefix}class {name}:\n')
            write_members_recursively(member, prefix + '    ', inc_docs, file)
        elif inspect.ismethoddescriptor(member):
            docs = member.__doc__.split("\n")
            file.write(f'{prefix}def {get_first_signature_line_from_docs(docs)}:{suffix}\n')
            if inc_docs:
                file.write(f'{prefix + "    "}"""')
                for line in docs[1:]:
                    file.write(f'{prefix + "    "}{line}\n')
                file.write(f'{prefix + "    "}"""\n')
        else:
            file.write(f'{prefix}{name}: {type(member).__name__}\n')


def main() -> int:
    out_path = os.path.join(os.path.dirname(__file__), 'stubs', 'svfsc.pyi')
    print(f'Writing to {out_path}')
    with open(out_path, 'w') as file:
        file.write(
            f'# Auto-generated from svfsc version {svfsc.SVFS_CPP_VERSION}'
            f' by {os.path.basename(__file__)}'
            f' at {datetime.datetime.now(datetime.timezone.utc)} UTC'
            f'\n'
        )
        file.write('import typing\n')
        file.write('import datetime\n')
        file.write('\n')
        write_members_recursively(svfsc, '', False, file)
    print(f'DONE')
    return 0


if __name__ == '__main__':
    exit(main())
