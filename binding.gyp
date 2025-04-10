{
    "targets": [{
        "target_name": "camera_sabotage_detector",
        "cflags!": ["-fno-exceptions"],
        "cflags_cc!": ["-fno-exceptions", "-fno-rtti"],
        "sources": ["src/camera_sabotage_detector.cpp"],
        "include_dirs": [
            "<!@(node -p \"require('node-addon-api').include\")",
            "/usr/local/include/opencv4",
            "/usr/include/opencv4"
        ],
        "libraries": [
            "-lopencv_core",
            "-lopencv_imgcodecs",
            "-lopencv_imgproc"
        ],
        "defines": ["NAPI_DISABLE_CPP_EXCEPTIONS"],
        "conditions": [
            ["OS=='mac'", {
                "xcode_settings": {
                    "GCC_ENABLE_CPP_EXCEPTIONS": "YES",
                    "GCC_ENABLE_CPP_RTTI": "YES",
                    "CLANG_CXX_LIBRARY": "libc++",
                    "MACOSX_DEPLOYMENT_TARGET": "10.15"
                },
                "include_dirs+": [
                    "/opt/homebrew/include/opencv4",
                    "/usr/local/include/opencv4"
                ],
                "libraries": [
                    "-L/opt/homebrew/lib",
                    "-L/usr/local/lib",
                    "-lopencv_core",
                    "-lopencv_imgproc",
                    "-lopencv_imgcodecs"
                ]
            }],
            ["OS=='linux'", {
                "include_dirs+": [
                    "<!@(pkg-config --cflags opencv4 2>/dev/null || pkg-config --cflags opencv 2>/dev/null | sed 's/^-I//')"
                ],
                "libraries": [
                    "<!@(pkg-config --libs opencv4 2>/dev/null || pkg-config --libs opencv 2>/dev/null)"
                ],
                "cflags+": [
                    "<!@(pkg-config --cflags opencv4 2>/dev/null || pkg-config --cflags opencv 2>/dev/null)"
                ]
            }]
        ],
        "msvs_settings": {
            "VCCLCompilerTool": {
                "RuntimeTypeInfo": "true"
            }
        }
    }]
}
