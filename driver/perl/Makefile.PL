use 5.008007;
use ExtUtils::MakeMaker;
# See lib/ExtUtils/MakeMaker.pm for details of how to influence
# the contents of the Makefile that is written.
my $CC = "g++";
WriteMakefile(
    NAME              => 'QConf',
    VERSION_FROM      => 'lib/QConf.pm', # finds $VERSION
    PREREQ_PM         => {}, # e.g., Module::Name => 1.1
    ($] >= 5.005 ?     ## Add these new keywords supported since 5.005
      (ABSTRACT_FROM  => 'lib/QConf.pm', # retrieve abstract from module
       AUTHOR         => 'root <root@localdomain>') : ()),
    LIBS              => ['-L./ -lqconf'], # e.g., '-lm'
    DEFINE            => '', # e.g., '-DHAVE_SOMETHING'
    INC               => '-I.', # e.g., '-I. -I/usr/include/other'
    # Un-comment this if you add C files to link with later:
    # OBJECT            => '$(O_FILES)', # link all the C files too
    'CC'=>$CC,
    'LD'=>'$(CC)',
    'XSOPT'=>'-C++',
    'TYPEMAPS'=>['perlobject.map'],
);
