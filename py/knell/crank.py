"""
A tool in which commands can be dispatched and executed.
"""
import argparse
import cmd
import json
import multiprocessing
import os
import shlex
import shutil
import subprocess
import sys
from typing import Optional

from .context import ProjectContext
from .multiplatform import root_to_executable, root_to_shared_lib
from .parsers import *
from .run import run_program


def generator_settings_for_compiler(cmake_path: str, compiler_path: Optional[str]):
    """Makes settings to give the generator for a specific compiler."""
    settings = []
    if compiler_path is not None:
        settings = [f'-DCMAKE_C_COMPILER={compiler_path}']

    # https://cmake.org/cmake/help/latest/generator/Visual%20Studio%2015%202017.html
    if sys.platform == 'win32':
        if compiler_path is None:
            arch = 'x64'
            help_output = subprocess.check_output([cmake_path, '--help'])
            generator = None
            for line in help_output.decode().splitlines():
                if line.startswith('*'):
                    print(line)
                    generator = line[1:line.index('=')]
                    if '[arch]' in generator:
                        generator = generator.replace('[arch]', '')
                    generator = generator.strip()
                    print(f'"{generator}"')
                    break
            if generator is not None:
                settings.extend(['-G', generator, '-A', arch])
        else:
            settings.extend(['-G', 'Unix Makefiles'])

    return settings


def verify_executable_exists(ctx: ProjectContext, alias: Optional[str]) -> bool:
    """Return true if an alias maps to a file in a context and provide user messaging."""
    if alias is None:
        print(f'Cannot find an executable for a null alias.')
        return False

    if not ctx.has_registered_program(alias):
        print(f'No alias exists for {alias}')
        print('Use `crank register ALIAS PATH` to register a program for use.')
        print('Example: crank register cmake "C:/Program Files/CMake/bin/cmake.exe"')
        return False
    program_path = ctx.path_for_program(alias)
    if not os.path.isfile(program_path):
        print(f'Executable path for {alias} does not exist at {program_path}')
        return False
    return True


def verify_build_dir_exists(build_dir: str) -> bool:
    """Return true if the build dir exists while providing user messaging."""
    if os.path.isfile(build_dir):
        print(f'Build directory {build_dir} exists as something other than a directory')
        return False

    if not os.path.isdir(build_dir):
        print(f'Build directory {build_dir} does not exist')
        return False
    return True


def verify_venv_dir_exists(venv: str) -> bool:
    """Return true if the virtual environment dir exists while providing user messaging."""
    if os.path.isfile(venv):
        print(f'Virtual environment {venv} exists as something other than a directory')
        return False

    if not os.path.isdir(venv):
        print(f'Virtual environment {venv} does not exist')
        return False
    return True


def cmd_clean(ctx: ProjectContext, args: argparse.Namespace) -> int:
    build_dir: str = ctx.build_dir()
    if not os.path.exists(build_dir):
        return 0

    if not os.path.isdir(build_dir):
        print(f'Build directory {build_dir} is not a directory')
        return 1

    if args.dry_run:
        print(f'Dry run')
        print(f'Would have removed: {build_dir}')
    else:
        print(f'Removing: {build_dir}')
        try:
            shutil.rmtree(build_dir)
        except OSError as err:
            print(f'Error removing {build_dir} {str(err)}')
    return 0


def cmd_gen(ctx: ProjectContext, args: argparse.Namespace) -> int:
    if not verify_executable_exists(ctx, 'cmake'):
        return 1

    if ctx.compiler() is not None and not verify_executable_exists(ctx, ctx.compiler()):
        return 1

    build_dir = ctx.build_dir()
    if os.path.isfile(build_dir):
        print(f'Build directory {build_dir} exists as something other than a directory')
        return 1
    if os.path.isdir(build_dir):
        if args.force:
            if args.dry_run:
                print(f'Would have removed previously existing directory {build_dir}')
            else:
                shutil.rmtree(build_dir)
        else:
            print(f'{build_dir} exists.  Use --force to wipe and recreate the build dir.')
            return 1

    if args.dry_run:
        print(f'Would have created {build_dir}')
    else:
        print(f'Creating build directory {build_dir}')
        os.mkdir(build_dir)

    cmake_path: str = ctx.path_for_program('cmake')
    cmake_args = [cmake_path, '..']
    if args.enable_ccache:
        cmake_args.append('-DKN_ENABLE_CCACHE=1')

    compiler = ctx.compiler()
    if compiler is not None:
        compiler = ctx.path_for_program(compiler)
    cmake_args.extend(generator_settings_for_compiler(cmake_path, compiler))

    if args.dry_run:
        print(f'Would have run {cmake_args} in {build_dir}')
        return 0
    else:
        return run_program(cmake_args, cwd=build_dir)


def cmd_build(ctx: ProjectContext, args: argparse.Namespace) -> int:
    """Build using the current project configuration."""
    if not verify_executable_exists(ctx, 'cmake'):
        return 1

    if not verify_build_dir_exists(ctx.build_dir()):
        return 1

    cmake_args = [ctx.path_for_program('cmake'), '--build', '.',
                  '--parallel', str(multiprocessing.cpu_count()),
                  '--config', ctx.build_config()]

    if args.dry_run:
        print(f'Would have run {cmake_args} in {ctx.build_dir()}')
        return 0
    else:
        return run_program(cmake_args, cwd=(ctx.build_dir()))


def cmd_check(ctx: ProjectContext, args: argparse.Namespace) -> int:
    if not verify_executable_exists(ctx, 'cmake'):
        return 1

    if not verify_build_dir_exists(ctx.build_dir()):
        return 1

    check_target: str = 'check'
    if args.iterate:
        check_target += '-iterate'

    cmake_args = [ctx.path_for_program('cmake'), '--build', '.',
                  '--target', check_target,
                  '--config', ctx.build_config()]
    if args.dry_run:
        print(f'Would have run {cmake_args} in {ctx.build_dir()}')
        return 0
    else:
        return run_program(cmake_args, cwd=(ctx.build_dir()))


def cmd_demo(ctx: ProjectContext, args: argparse.Namespace) -> int:
    return 1


def cmd_run(ctx: ProjectContext, args: argparse.Namespace) -> int:
    ovr_ctx = ctx.copy_with_overrides(vars(args))

    # Ensure an up-to-date build.
    cmd_build(ovr_ctx, args)

    ovr_ctx.set_game(os.path.join(ctx.demo_dir(), root_to_shared_lib(args.demo)))
    return ovr_ctx.run_driver()


def cmd_env(ctx: ProjectContext, _args: argparse.Namespace) -> int:
    json.dump(ctx.dump(), sys.stdout, indent=4)
    return 0


def cmd_register(ctx: ProjectContext, args: argparse.Namespace) -> int:
    """Registers a new program for use in the given context."""
    if os.path.isfile(args.path) or args.force:
        if args.dry_run:
            if ctx.has_registered_program(args.alias) and not args.override:
                print(f'Trying to override {ctx.path_for_program(args.alias)} of {args.alias} with {args.path}')
                return 1
            else:
                print(f'Would have added alias {args.alias} -> {args.path}')
                return 0
        else:
            if ctx.register_program(args.alias, args.path, args.override):
                return 0
            return 1

    print(f'No program exists at {args.path}.')
    return 1


def cmd_default(ctx: ProjectContext, args: argparse.Namespace) -> int:
    return 1


def cmd_pysetup(ctx: ProjectContext, args: argparse.Namespace) -> int:
    """Creates a virtual environment for helper module installs."""
    if not verify_executable_exists(ctx, 'python3'):
        return 1

    if os.path.isfile(ctx.venv_dir()):
        print(f'Cannot create venv, {ctx.venv_dir()} is a file.')
        return 1

    if os.path.isdir(ctx.venv_dir()) and args.clean:
        shutil.rmtree(ctx.venv_dir())

    if not os.path.isdir(ctx.venv_dir()):
        venv_args = [(ctx.path_for_program('python3')), '-m', 'venv', ctx.venv_dir()]
        venv_setup = run_program(venv_args, cwd=ctx.knell_home())
        if venv_setup != 0:
            print(f'Could not create virtual environment at {ctx.venv_dir()}')
            return venv_setup
        ctx.register_program('localpython3', os.path.join(ctx.venv_dir(), 'Scripts', root_to_executable('python')),
                             override=True)

    pip_upgrade_result = run_program(
        [(ctx.path_for_program('localpython3')), '-m', 'pip', 'install', '--upgrade', 'pip'], cwd=ctx.knell_home())
    if pip_upgrade_result != 0:
        print('Could not upgrade pip.')

    requirements_file = os.path.join(ctx.py_dir(), 'requirements.txt')
    if os.path.isfile(requirements_file):
        pip_install_args = [ctx.path_for_program('localpython3'), '-m', 'pip', 'install', '-r', requirements_file]
    else:
        required_dev_packages = ['mypy', 'pylint', 'pydocstyle', 'pycodestyle', 'bandit', 'colorama']
        pip_install_args = [ctx.path_for_program('localpython3'), '-m', 'pip', 'install']
        pip_install_args.extend(required_dev_packages)
    return run_program(pip_install_args, cwd=ctx.knell_home())


def py_files():
    return ['knell']


def cmd_pycheck(ctx: ProjectContext, args: argparse.Namespace) -> int:
    """Run all the checks on Python source."""
    if not verify_executable_exists(ctx, 'localpython3'):
        return 1

    if not verify_venv_dir_exists(ctx.venv_dir()):
        return 1

    python_path = ctx.path_for_program('localpython3')
    checks = [['mypy'],
              ['pylint'],
              ['pycodestyle', '--max-line-length=120', '--show-source', '--statistics', '--count'],
              ['pydocstyle', '--ignore=D200,D203,D204,D212,D401'],
              ]

    failures = 0
    for program in checks:
        cmd_line = [python_path, '-m']
        cmd_line.extend(program)
        cmd_line.extend(py_files())
        if run_program(cmd_line, cwd=ctx.py_dir()) != 0:
            failures += 1
            if args.incremental:
                return failures

    return failures


def cmd_source(ctx: ProjectContext, args: argparse.Namespace) -> int:
    return 1


def cmd_save(ctx: ProjectContext, args: argparse.Namespace) -> int:
    ctx.save()
    return 0


def cmd_load(ctx: ProjectContext, args: argparse.Namespace) -> int:
    return 1


COMMAND_PARSERS = {
    # Build and test
    'clean': (parser_clean, cmd_clean, 'Remove build directories'),
    'gen': (parser_gen, cmd_gen, 'Generate build types'),
    'build': (parser_build, cmd_build, 'Do a build'),
    'check': (parser_check, cmd_check, 'Run tests'),

    # Run
    'demo': (parser_demo, cmd_demo, 'List built demos available to be run'),
    'run': (parser_run, cmd_run, 'Run a demo or a game.'),

    # Environment
    'env': (parser_env, cmd_env, 'Prints the current tool configuration'),
    'register': (parser_register, cmd_register, 'Change program registration.'),
    'default': (parser_default, cmd_default, 'Set default configuration parameters.'),

    # Command history and automation
    'source': (parser_source, cmd_source, 'Run each line from a file as a command.'),
    'save': (parser_save, cmd_save, 'Save configuration to file.'),
    'load': (parser_load, cmd_load, 'Load configuration from a file.'),

    # Development
    'pysetup': (parser_pysetup, cmd_pysetup, 'Setup virtual environment for subtools.'),
    'pycheck': (parser_pycheck, cmd_pycheck, 'Run Python linting and testing.'),
}


def parse_args() -> argparse.Namespace:
    """
    Parse command arguments and return a namespace for creating a project context.
    """
    parser = argparse.ArgumentParser()
    commands = parser.add_subparsers(dest='command')
    for command in COMMAND_PARSERS:
        subparser = commands.add_parser(command, help=COMMAND_PARSERS[command][2])
        COMMAND_PARSERS[command][0](subparser)
    parser_add_top_level_args(parser)

    args = parser.parse_args()
    if args.command is None:
        parser.print_help()
        sys.exit(0)

    return args


def map_help(action):
    help = lambda self: action.__doc__
    return help


class CmdArgumentParser(argparse.ArgumentParser):
    def parse_args(self, args):
        """Catch the SystemExit so bad parses don't crash the interactive session."""
        try:
            return super().parse_args(args)
        except SystemExit:
            return None


def map_command(self, command, parser, action):
    # Double-parser args because this needs to fit into a lambda.
    # Shortcircuit on argument parsing to prevent command functions from needing
    # to perform error handling relating to argument parsing.
    wrap = lambda args: parser(CmdArgumentParser(usage=command)).parse_args(shlex.split(args)) is not None and action(
        self.ctx, parser(CmdArgumentParser(usage=command)).parse_args(shlex.split(args))) and False
    wrap.__doc__ = action.__doc__
    return wrap


class InteractiveMode(cmd.Cmd):
    """Handler for Crank's interactive mode."""

    def __init__(self, ctx):
        """Initialize interactive mode with a known context."""
        self._names = []
        self.ctx = ctx
        self.parser = CmdArgumentParser()
        for command in COMMAND_PARSERS:
            parser = COMMAND_PARSERS[command][0]
            action = COMMAND_PARSERS[command][1]
            setattr(self, f'do_{command}', map_command(self, command, parser, action))
            self._names.append(f'do_{command}')
        parser_add_top_level_args(self.parser)
        super().__init__()

    def get_names(self):
        """
        Overridden to provide support for dynamically adding commands.

        cmd.Cmd uses dir(self.__class__) which won't include our monkey-patched
        methods in the default command list.
        """
        return self._names

    def do_quit(self, _arg):
        return True

    def do_exit(self, _arg):
        return True


def default_knell_home():
    """Knell is assumed to be provided by the environment, or the current directory."""
    return os.environ.get('KNELL_HOME', os.getcwd())


def run_interactive_loop():
    """Starts an interactive terminal."""
    print('Running interactively.')

    # Establish the target environment for the script.
    knell_home: str = default_knell_home()

    # Build the context using the given home directory.
    ctx: ProjectContext = ProjectContext(knell_home)

    repl = InteractiveMode(ctx)
    repl.cmdloop()

    # if repl.reload:
    #     kn.reload()
    # else:
    #     sys.exit(repl.last_exit_code)


def run_as_script():
    """Runs Crank as a simple command using the current command line."""

    # `args` is a namespace of all of our possible arguments, appropriate for
    # whatever command is going to be executed.  This isn't going to be a
    # suitable thing for passing around to each command though.
    args = parse_args()

    # Establish the target environment for the script.
    knell_home: str = default_knell_home()
    if args.knell_home:
        knell_home = args.knell_home

    # Build the context using the given home directory.
    ctx: ProjectContext = ProjectContext(knell_home)
    ctx = ctx.copy_with_overrides(vars(args))

    # Running in non-interactive mode.
    # Dispatch to the appropriate handling function.
    retval = COMMAND_PARSERS[args.command][1](ctx, args)

    if args.command in ['register', 'pysetup']:
        ctx.save()

    return retval
