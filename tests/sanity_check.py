#!/usr/bin/env python3
# -*- coding: utf-8 -*-

""" Sanity check for andiff and anpatch applications

"""

import os
import time
import tempfile
import hashlib
import logging
import itertools
import argparse
import subprocess

from sys import stdout


TMP_LOCATION = '/tmp'
""" Location of temporary directory """


class CmdColors:
    """ Simple class to color printed output.
    Print method will color output iff console is attached to stdout.
    This should prevent the situation when output is logged into file.

    """
    OK_GREEN = '\033[92m'
    WARNING = '\033[93m'
    FAIL = '\033[91m'
    END = '\033[0m'

    @staticmethod
    def __make_color(color, text):
        """ Here's actual magic happen.

        Returns:
            str: Colored text
        """
        if stdout.isatty():
            return color + text + CmdColors.END
        return text

    @staticmethod
    def make_red(text):
        """ Return text in red

        Returns:
            str: Colored text
        """
        return CmdColors.__make_color(CmdColors.FAIL, text)

    @staticmethod
    def make_green(text):
        """ Return text in green

        Returns:
            str: Colored text
        """
        return CmdColors.__make_color(CmdColors.OK_GREEN, text)


def create_tmp_file(tmp_dir, file_size):
    """ Create temporary file with optional size

    Args:
        tmp_dir: Directory where file should be created
        file_size: Size of temporary file

    Returns:
        str: Created filename
    """
    tmp_file_fd, tmp_file = tempfile.mkstemp(dir=tmp_dir)

    if file_size == 0:
        os.close(tmp_file_fd)
        return tmp_file
    tmp_data = os.urandom(file_size)
    os.write(tmp_file_fd, tmp_data)
    os.close(tmp_file_fd)
    del tmp_data
    return tmp_file


def calculate_file_hash(filename):
    """ Calculate MD5 check-sum for given file

    Args:
        filename: Location of file

    Returns:
        str: MD5 check-sum
    """
    with open(filename, 'rb') as file:
        return hashlib.md5(file.read()).hexdigest()


def run_application(args):
    """ Helper function to run external application and print
    execution time.

    Args:
        args[List]: Application with all arguments
    """
    start = time.time()

    subprocess.check_call(args, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)

    done = time.time()
    elapsed = done - start
    logging.debug('Command took %fs', elapsed)


def run_test(tmp_dir, files_size, andiff_app, anpatch_app):
    """ Run actual test

    Args:
        tmp_dir: Temporary directory for test
        files_size: Size of temporary file in bytes
        andiff_app: Location of andiff app
        anpatch_app: Location of anpatch app
    """
    source_file = create_tmp_file(tmp_dir=tmp_dir, file_size=files_size)
    logging.debug('Creating source file %s of size %s KB', source_file, files_size)

    target_file = create_tmp_file(tmp_dir=tmp_dir, file_size=files_size)
    logging.debug('Creating target file %s of size %s KB', target_file, files_size)

    patch_file = create_tmp_file(tmp_dir=tmp_dir, file_size=0)
    logging.debug('Patch file has been created: %s', patch_file)

    logging.debug('Running andiff')
    run_application((andiff_app, source_file, target_file, patch_file))

    patched_file = create_tmp_file(tmp_dir=tmp_dir, file_size=0)
    logging.debug('Patched file has been created: %s', patched_file)

    logging.debug('Running anpatch')
    run_application((anpatch_app, source_file, patched_file, patch_file))

    logging.debug('Calculating hashes')
    target_file_md5 = calculate_file_hash(target_file)
    logging.debug('Target file: %s', target_file_md5)

    patched_file_md5 = calculate_file_hash(patched_file)
    logging.debug('Patched file: %s', patched_file_md5)

    if patched_file_md5 == target_file_md5:
        logging.info('Result: ' + CmdColors.make_green('OK'))
    else:
        logging.critical('Result: ' + CmdColors.make_red('FAIL'))
        raise Exception('Something went wrong. Leaving broken files')

    for file_to_remove in [target_file, source_file, patched_file, patch_file]:
        os.unlink(file_to_remove)


def main():
    """ Main program function """

    parser = argparse.ArgumentParser(description='Test andiff and anpatch application')
    parser.add_argument('--diff', metavar='andiff', type=str, required=True,
                        help='Location of andiff application')
    parser.add_argument('--patch', metavar='anpatch', type=str, required=True,
                        help='Location of anpatch application')
    parser.add_argument('--size', type=int, default=10, help='Size of test file')
    parser.add_argument('--repeat', type=int, default=1, help='Repeat test n times')
    parser.add_argument('-v,--verbose', dest='verbose', action='store_true',
                        help='Repeat test n times')

    args = parser.parse_args()

    logging_level = logging.INFO
    logging_format = '%(message)s'

    if args.verbose:
        logging_level = logging.DEBUG
        logging_format = '%(asctime)s %(message)s'

    logging.basicConfig(format=logging_format, level=logging_level)

    logging.debug(args)

    print('Start andiff sanity check')

    logging.debug('Output is attached to console: ' + str(stdout.isatty()))

    andiff_app = os.path.abspath(args.diff)
    anpatch_app = os.path.abspath(args.patch)

    logging.debug('andiff location: %s', andiff_app)
    logging.debug('anpatch location: %s', anpatch_app)

    tmp_dir = tempfile.mkdtemp(prefix='andiff', dir=TMP_LOCATION)
    logging.debug('Temporary directory: %s', tmp_dir)

    files_size = args.size * 1024 * 1024

    for _ in itertools.repeat(None, args.repeat):
        run_test(tmp_dir=tmp_dir, files_size=files_size,
                 andiff_app=andiff_app, anpatch_app=anpatch_app)

    os.rmdir(tmp_dir)


if __name__ == '__main__':
    main()
