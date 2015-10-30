{
"targets": [
    {
      "target_name": "stata",
      "type" : "shared_library",
      "sources": [ "stata/stata.c",
                   "stata/statawrite.c",
                   "stata/stataread.c",
                   "stata/stataread.h",
                   "stata/swap_bytes.h",
                   "stata/stata.h"],
        "libraries" : [ '../libjansson.a'],
        "cflags" : [ "-fPIC" ],
    }
    ]
}
