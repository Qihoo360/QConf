from distutils.core import setup, Extension
setup(name = 'qconf_py', version = '1.2.2', ext_modules = [Extension('qconf_py', ['lib/python_qconf.cc'],
     include_dirs=['/usr/local/qconf/include'],
     extra_objects=['/usr/local/qconf/lib/libqconf.a']
     )])
