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
        "libraries" : [ 'libjansson.a'],
        "cflags" : [ "-fPIC" ],
    }
    ]
}
