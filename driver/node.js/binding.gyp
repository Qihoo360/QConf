{
    "targets": [
        {
            "target_name": "qconf",
            "sources": [ "qconf.cc" ],
            "cflags!": ["-fno-exceptions"],
            "cflags_cc!": ["-fno-exceptions"],
            "libraries": ["-lqconf"],
            "link_settings": {
            "libraries": ["-L<!(echo $QCONF_INSTALL)/lib"]
            },
            "include_dirs": [
                "<!(echo $QCONF_INSTALL)/include",
                "/usr/local/qconf/include",
                "/usr/local/include/qconf"
            ],
            "copies": [
                {
                    'destination': '/usr/local/qconf/lib',
                    'files': [
                        './build/Release/qconf.node'
                    ]
                }
            ]
        }
    ]
}
