{
"targets": [
    {
      "target_name": "stata",
      "type" : "shared_library",
      "sources": [ "stata.c",
                   "statawrite.c",
                   "stataread.c",
                   "stataread.h",
                   "swap_bytes.h",
                   "stata.h"],
        "libraries" : [ '<!(pwd)/libjansson.a'],
        "ldflags" : ['-Wl,-rpath,<!(pwd)'],
        "cflags" : [ "-fPIC" ],
    }
    ]
}
