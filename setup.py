from distutils.core import setup, Extension

pylorcon = Extension('pylorcon',
		sources = ['pylorcon.c'],
		libraries = ['orcon'])

setup(name = 'pylorcon',
		version = '1.0',
		description = 'Python wrapper for LORCON',
		author = 'Tom Wambold',
		author_email = 'tom5760@gmail.com',
		ext_modules = [pylorcon])
