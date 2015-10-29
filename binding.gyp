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
        "libraries" : [ '-ljansson', '-L/usr/lib/x86_64-linux-gnu']
    }
    ]
}
