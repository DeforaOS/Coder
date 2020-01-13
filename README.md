DeforaOS Coder
==============

About Coder
-----------

Coder contains the following components:
 * coder(1), an experimental Integrated Development Environment (IDE) for the
   DeforaOS Project (currently useless)
 * console(1), a console for programming languages (only supports PHP for the
   moment)
 * debugger(1), a graphical interface for debugging programs
 * gdeasm(1), a disassembler
 * sequel(1), a console for SQL databases
 * simulator(1), a graphical interface to simulate software on embedded devices

Coder is part of the DeforaOS Project, found at https://www.defora.org/.

Compiling Coder
---------------

The current requirements for compiling Keyboard are as follows:
 * DeforaOS Asm
 * DeforaOS CPP
 * DeforaOS libDatabase
 * DeforaOS libDesktop
 * Gtk+ 2.4 or later, or Gtk+ 3.0 or later
 * an implementation of `make`
 * gettext (libintl) for translations

With these installed, the following command should be enough to compile Coder
on most systems:

    $ make

The following command will then install Coder:

    $ make install

To install (or package) Coder in a different location:

    $ make clean
    $ make PREFIX="/another/prefix" install

Coder also supports `DESTDIR`, to be installed in a staging directory; for
instance:

    $ make DESTDIR="/staging/directory" PREFIX="/another/prefix" install

On some systems, the Makefiles shipped can be re-generated accordingly thanks to
the DeforaOS configure tool.

The compilation process supports a number of options, such as PREFIX and DESTDIR
for packaging and portability, or OBJDIR for compilation outside of the source
tree.

Distributing Coder
------------------

DeforaOS Coder is subject to the terms of the GNU GPL license, version 3. Please
see the `COPYING` file for more information.
