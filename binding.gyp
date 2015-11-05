{
"targets": [
    {
      "target_name": "stata",
      "type" : "shared_library", 
      "product_extension" : "so",
      "sources": [ "stata.c",
                   "statawrite.c",
                   "stataread.c",
                   "stataread.h",
                   "swap_bytes.h",
                   "stata.h"],
        "libraries" : [ '<!(pwd)/libjansson.a'],
        "ldflags" : ['-Wl,-rpath,<!(pwd)'],
        "cflags" : [ "-fPIC" ],
    },
    {
        "target_name" : "copy_module",
        "type" : "none",
        "copies" : [
                {
                  "files": [ "<(LIB_DIR)/stata.so"   ],
                  "destination": "<!(pwd)"
                }
            ]
    }
    ]
    
}
