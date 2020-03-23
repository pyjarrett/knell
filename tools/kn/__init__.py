import argparse
import cmd
import os
import multiprocessing
import queue
import shutil
import subprocess
import sys
import threading
import time


def git_branch():
    return subprocess.check_output('git branch --show-current'.split()).decode().strip()


def generate_prompt():
    return f'(knife {git_branch()}) '


# TODO: Should support building for clang, gcc and MSVC, wherever each one is available.
def supported_compilers():
    return ['default', 'clang', 'gcc']


def supported_commands():
    return ['gen']


def build_dir_for_compiler(compiler):
    if compiler == 'default' or compiler is None:
        return 'build'
    else:
        return f'build-{compiler}'


KN_CONFIG_COMPILER = 'compiler'
KN_CONFIG_KEYS = [KN_CONFIG_COMPILER]

def cmake_compiler_generator_settings(compiler):
    settings = []
    if compiler != 'default':
        settings = [f'-DCMAKE_C_COMPILER={compiler}']

    # https://cmake.org/cmake/help/latest/generator/Visual%20Studio%2015%202017.html
    if sys.platform == 'win32' and (compiler is None or compiler == 'default'):
        help_output = subprocess.check_output(['cmake', '--help'])
        for line in help_output.decode().splitlines():
            if line.startswith('*'):
                print(line)
                generator = line[1:line.index('=')]
                if '[arch]' in generator:
                    generator = generator.replace('[arch]', '')
                generator = generator.strip()
                print(f'"{generator}"')
                break
        settings.extend(['-G', generator, '-A', 'x64'])

    return settings


def read_stream(stream, q):
    for line in stream:
        q.put(line.decode().strip())


def run_program(command_line_array, **kwargs):
    """
    Runs another process while streaming its stdout and stderr.
    """
    print(f'Running: {command_line_array} {kwargs}')
    process = subprocess.Popen(command_line_array, stdout=subprocess.PIPE, stderr=subprocess.PIPE, **kwargs)

    out_queue = queue.Queue()
    err_queue = queue.Queue()

    out_thread = threading.Thread(target=read_stream, args=(process.stdout, out_queue))
    err_thread = threading.Thread(target=read_stream, args=(process.stderr, out_queue))

    out_thread.start()
    err_thread.start()

    while out_thread.is_alive() or err_thread.is_alive() and not out_queue.empty() or not err_queue.empty():
        try:
            line = out_queue.get_nowait()
            print(line)
        except queue.Empty:
            pass

        try:
            line = err_queue.get_nowait()
            print(f'err:{line}')
        except queue.Empty:
            pass

    out_thread.join()
    err_thread.join()

    process.wait()


class BuildContext():
    def __init__(self):
        self.config = {}

    def values(self):
        return self.config

    def build_dir(self):
        return build_dir_for_compiler(self.config.get('compiler'))

    def compiler(self):
        return self.config.get('compiler')

    def parse(self, args):
        parser = argparse.ArgumentParser(usage='key value\n    Sets a key equal to a value.\n')
        parser.add_argument('key', choices=['compiler'])
        parser.add_argument('value')

        try:
            args = parser.parse_args(args.split())
            self.config[args.key] = args.value
        except SystemExit:
            pass


class Knife(cmd.Cmd):
    """
    Interactive command-line tool to simplify development with Knell.  This
    allows building, testing, and running knell consistently across platforms.
    """
    prompt = generate_prompt()
    intro = 'Knife: A tool to help with Knell building, running, and debugging\n'

    def __init__(self, reload_fn):
        super().__init__()
        self.context = BuildContext()
        self.reload_fn = reload_fn
        self.reload = False
        self.cmd_start_time = 0

    @staticmethod
    def update_prompt():
        Knife.prompt = generate_prompt()

    def precmd(self, line):
        self.cmd_start_time = time.monotonic()
        return line

    def postcmd(self, stop, line):
        now = time.monotonic()
        print(f'{line} {now - self.cmd_start_time:>8.2f}s')

        if not stop:
            Knife.update_prompt()
            return False
        else:
            return True

    def do_quit(self, arg):
        self.reload = False
        return True

    def do_exit(self, arg):
        """Alias for quit."""
        self.reload = False
        return True

    def do_reload(self, args):
        """[For Development] Reloads knife with updated source."""
        self.reload_fn()
        self.reload = True
        return True

    def do_last_commit(self, arg):
        """Prints the hash and short log of the last commit."""
        print(subprocess.check_output('git log -1 --pretty=format:%h:%s'.split()).decode())

    def do_config(self, arg):
        """Print the current state of configuration variables."""
        for key in self.context.values():
            print(f'{key:<15}{self.context.values()[key]:<}')
        print(f'Build Dir: {self.context.build_dir()}')
        print(f'Compiler:  {self.context.compiler() if self.context.compiler() else "Default"}')

    def do_set(self, args):
        """
        Sets configuration values.
        """
        self.context.parse(args)

    def do_clean(self, args):
        """
        Wipes build directory.
        """
        build_dir = self.context.build_dir()
        if os.path.exists(build_dir):
            if not os.path.isdir(build_dir):
                print(f'Build directory {build_dir} exists as something other than a directory')
                return
            else:
                print(f'Wiping the build directory {build_dir}')
                shutil.rmtree(build_dir)

    def do_gen(self, args):
        """
        Generate project files.
        """
        build_dir = self.context.build_dir()
        if os.path.exists(build_dir):
            if not os.path.isdir(build_dir):
                print(f'Build directory {build_dir} exists as something other than a directory')
                return
            elif '--force' in words:
                print(f'Wiping the build directory {build_dir}')
                shutil.rmtree(build_dir)
            else:
                print(f'{build_dir} exists.  Rerun with --force to remove the existing directory')
                return

        print(f'Creating build directory {build_dir}')
        os.mkdir(build_dir)
        cmake_args = ['cmake', '..']

        compiler = self.context.compiler()
        if compiler is not None:
            cmake_args.extend(cmake_compiler_generator_settings(compiler))
        print(cmake_args)
        run_program(cmake_args, cwd=build_dir)

    def do_build(self, args):
        """
        Builds using the current project configuration.
        """
        words = args.split()
        compiler = None
        if len(words) >= 1:
            compiler = words.pop(0)
        elif len(words) == 0:
            compiler = 'default'
        else:
            print(f'Cannot build with {args}')
            return

        build_dir = build_dir_for_compiler(compiler)
        if not os.path.exists(build_dir):
            print(f'Build for {compiler} does not exist at {build_dir}')
            return

        cmake_args = ['cmake', '--build', '.', '--parallel', str(multiprocessing.cpu_count())]
        run_program(cmake_args, cwd=build_dir)

    def do_check(self, args):
        """
        Runs all tests.
        """
        words = args.split()
        compiler = None
        if len(words) >= 1:
            compiler = words.pop(0)
        elif len(words) == 0:
            compiler = 'default'
        else:
            print(f'Cannot build with {args}')
            return

        build_dir = build_dir_for_compiler(compiler)
        if not os.path.exists(build_dir):
            print(f'Build for {compiler} does not exist at {build_dir}')
            return
        run_program(['cmake', '--build', '.', '--target', 'check'], cwd=build_dir)

    def do_check_iterate(self, args):
        """
        Runs only failed tests.
        """
        build_dir = self.context.build_dir()
        if not os.path.exists(build_dir):
            print(f'Build for {compiler} does not exist at {build_dir}')
            return
        run_program(['cmake', '--build', '.', '--target', 'check-iterate'], cwd=build_dir)
